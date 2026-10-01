# USB bootstrap (once, after switching to the dual-OTA partition table):
#   idf.py fullclean
#   idf.py build
#   idf.py -p COMx erase-flash
#   idf.py -p COMx flash monitor
# Confirm serial log: two OTA slots, STA IP, "OTA server started".
#
# Subsequent updates over WiFi (same LAN as the ESP32 STA):
#   .\scripts\ota_upload.ps1 -Ip 192.168.1.50
#   or:  .\scripts\ota_upload.ps1 -HostName flipdot.local
#
# Token defaults to Kconfig CONFIG_OTA_PASSWORD ("flipdot-ota").
# Override with -Token or $env:OTA_TOKEN.

param(
    [string]$Ip,
    [string]$HostName,
    [int]$Port = 8080,
    [string]$Token = $env:OTA_TOKEN,
    [string]$Bin = "build\flip_dot_c.bin"
)

$ErrorActionPreference = "Stop"

if (-not $Token) {
    $Token = "flipdot-ota"
}

if (-not $Ip -and -not $HostName) {
    throw "Specify -Ip <address> or -HostName flipdot.local"
}

$target = if ($Ip) { $Ip } else { $HostName }
$uri = "http://${target}:${Port}/ota"

if (-not (Test-Path -LiteralPath $Bin)) {
    throw "Firmware not found: $Bin  (run idf.py build first)"
}

Write-Host "Checking OTA endpoint $uri"
try {
    $info = Invoke-WebRequest -Uri $uri -Method Get -TimeoutSec 5
    Write-Host $info.Content.Trim()
} catch {
    throw "Cannot reach $uri : $_"
}

Write-Host "Uploading $Bin"
$headers = @{ Authorization = "Bearer $Token" }
$response = Invoke-WebRequest -Uri $uri -Method Post -Headers $headers `
    -InFile $Bin -ContentType "application/octet-stream" -TimeoutSec 180
Write-Host $response.Content.Trim()
Write-Host "Upload finished. Device should reboot into the new image."
