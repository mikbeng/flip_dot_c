/**
 * @file ota_update.h
 * @brief HTTP push OTA server (POST firmware.bin to /ota)
 */

#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <stdbool.h>
#include "esp_err.h"

esp_err_t ota_update_start(void);
bool ota_update_is_active(void);
void ota_update_mark_running_valid(void);

#endif /* OTA_UPDATE_H */
