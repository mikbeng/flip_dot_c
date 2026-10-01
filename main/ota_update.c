/**
 * @file ota_update.c
 * @brief HTTP push OTA: POST /ota with Authorization: Bearer <token>
 */

#include "ota_update.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"
#include "sdkconfig.h"

#include "flip_dot.h"
#include "pwr_ctrl.h"

static const char *TAG = "ota_update";

static httpd_handle_t s_server;
static volatile bool s_ota_active;
static volatile bool s_server_started;

static void ota_idle_hardware(void)
{
    ESP_LOGI(TAG, "Idling flip-dot hardware for OTA");
    flip_dot_suspend_updates(true);
    disable_flip_board();
}

static void ota_resume_hardware(void)
{
    ESP_LOGI(TAG, "Restoring flip-dot hardware after failed OTA");
    enable_flip_board();
    flip_dot_suspend_updates(false);
}

static bool ota_auth_ok(httpd_req_t *req)
{
    const char *token = CONFIG_OTA_PASSWORD;
    if (token[0] == '\0') {
        ESP_LOGE(TAG, "CONFIG_OTA_PASSWORD is empty, refusing upload");
        return false;
    }

    size_t hdr_len = httpd_req_get_hdr_value_len(req, "Authorization");
    if (hdr_len == 0) {
        return false;
    }

    char *auth = malloc(hdr_len + 1);
    if (!auth) {
        return false;
    }

    if (httpd_req_get_hdr_value_str(req, "Authorization", auth, hdr_len + 1) != ESP_OK) {
        free(auth);
        return false;
    }

    char expected[160];
    snprintf(expected, sizeof(expected), "Bearer %s", token);
    bool ok = (strcmp(auth, expected) == 0);
    free(auth);
    return ok;
}

static void log_partition_info(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);
    const esp_app_desc_t *desc = esp_app_get_description();

    if (running) {
        ESP_LOGI(TAG, "Running partition: %s subtype %d at 0x%lx size 0x%lx",
                 running->label, running->subtype,
                 (unsigned long)running->address, (unsigned long)running->size);
    }
    if (update) {
        ESP_LOGI(TAG, "Next OTA partition: %s subtype %d at 0x%lx size 0x%lx",
                 update->label, update->subtype,
                 (unsigned long)update->address, (unsigned long)update->size);
    }
    if (desc) {
        ESP_LOGI(TAG, "App version: %s, IDF: %s", desc->version, desc->idf_ver);
    }
}

static esp_err_t ota_get_handler(httpd_req_t *req)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    const esp_partition_t *running = esp_ota_get_running_partition();
    char body[192];
    snprintf(body, sizeof(body), "flip_dot_c version=%s partition=%s\n",
             desc ? desc->version : "unknown",
             running ? running->label : "unknown");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, body);
    return ESP_OK;
}

static esp_err_t ota_post_handler(httpd_req_t *req)
{
    if (s_ota_active) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA already in progress");
        return ESP_FAIL;
    }

    if (!ota_auth_ok(req)) {
        ESP_LOGW(TAG, "OTA rejected: missing or invalid Authorization");
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_sendstr(req, "Unauthorized");
        return ESP_FAIL;
    }

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    if (req->content_len > 0 && (size_t)req->content_len > update_partition->size) {
        ESP_LOGE(TAG, "Image too large: %d > %lu", req->content_len,
                 (unsigned long)update_partition->size);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Image too large");
        return ESP_FAIL;
    }

    s_ota_active = true;
    ota_idle_hardware();

    ESP_LOGI(TAG, "Starting OTA to %s, content_len=%d",
             update_partition->label, req->content_len);

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        s_ota_active = false;
        ota_resume_hardware();
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_begin failed");
        return ESP_FAIL;
    }

    char buf[1024];
    int remaining = req->content_len;
    const bool unknown_len = (remaining <= 0);
    size_t written = 0;
    bool failed = false;

    while (unknown_len || remaining > 0) {
        int to_read = unknown_len ? (int)sizeof(buf)
                                  : remaining < (int)sizeof(buf) ? remaining : (int)sizeof(buf);
        int recvd = httpd_req_recv(req, buf, to_read);
        if (recvd == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (recvd <= 0) {
            if (unknown_len && recvd == 0) {
                break;
            }
            ESP_LOGE(TAG, "httpd_req_recv failed: %d", recvd);
            failed = true;
            break;
        }

        err = esp_ota_write(ota_handle, buf, recvd);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            failed = true;
            break;
        }

        written += (size_t)recvd;
        if (!unknown_len) {
            remaining -= recvd;
        }
        if ((written & 0xFFFF) == 0) {
            ESP_LOGI(TAG, "OTA written %u bytes", (unsigned)written);
        }

        if (written > update_partition->size) {
            ESP_LOGE(TAG, "Write exceeded partition size");
            failed = true;
            break;
        }
    }

    if (failed) {
        esp_ota_abort(ota_handle);
        s_ota_active = false;
        ota_resume_hardware();
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
        return ESP_FAIL;
    }

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        s_ota_active = false;
        ota_resume_hardware();
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid OTA image");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        s_ota_active = false;
        ota_resume_hardware();
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set boot partition");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "OTA complete (%u bytes), rebooting into %s",
             (unsigned)written, update_partition->label);
    httpd_resp_sendstr(req, "OTA OK, rebooting\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

static void start_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "mdns_init failed: %s", esp_err_to_name(err));
        return;
    }
    mdns_hostname_set(CONFIG_OTA_HOSTNAME);
    mdns_instance_name_set("Flip Dot Display");
    ESP_LOGI(TAG, "mDNS hostname: %s.local", CONFIG_OTA_HOSTNAME);
}

esp_err_t ota_update_start(void)
{
    if (s_server_started) {
        return ESP_OK;
    }

    log_partition_info();
    start_mdns();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_OTA_HTTP_PORT;
    config.ctrl_port = CONFIG_OTA_HTTP_PORT + 1;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 30;
    config.stack_size = 8192;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t get_uri = {
        .uri = "/ota",
        .method = HTTP_GET,
        .handler = ota_get_handler,
    };
    httpd_uri_t post_uri = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = ota_post_handler,
    };
    httpd_register_uri_handler(s_server, &get_uri);
    httpd_register_uri_handler(s_server, &post_uri);

    s_server_started = true;
    ESP_LOGI(TAG, "OTA server started on http://%s.local:%d/ota (POST firmware.bin)",
             CONFIG_OTA_HOSTNAME, CONFIG_OTA_HTTP_PORT);
    return ESP_OK;
}

bool ota_update_is_active(void)
{
    return s_ota_active;
}

void ota_update_mark_running_valid(void)
{
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_ota_mark_app_valid_cancel_rollback: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Running app marked valid, rollback cancelled");
}
