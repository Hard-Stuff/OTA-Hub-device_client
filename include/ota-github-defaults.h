#pragma once

#define OTA_SERVER "api.github.com"
#define OTA_PORT 443
#define OTA_CHECK_PATH "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/latest"
#define OTA_BIN_PATH "/repos/" OTAGH_OWNER_NAME "/" OTAGH_REPO_NAME "/releases/assets/"
#ifdef OTAGH_BEARER
#define OTA_BEARER OTAGH_BEARER
#endif
#define FIRMWARE_BIN_MATCH "firmware.bin"

#include <Arduino.h>
String OTA_ASSET_ENDPOINT_CONSTRUCTOR(String asset_id)
{
    return String(OTA_BIN_PATH) + asset_id;
}