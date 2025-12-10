#pragma once
#include <Arduino.h>
#include <Preferences.h>

namespace OTAHub::FOTA
{
#ifdef OTAHUB_BEARER_TOKEN
#pragma message("OTAHUB_BEARER_TOKEN is defined — bearer token will be saved to NVS at runtime.")
  void SaveBearerToNVS()
  {
    Preferences bearer_store;
    bearer_store.begin("ota_hub-bearer");
    bearer_store.putString("token", OTAHUB_BEARER_TOKEN);
    bearer_store.end();
    Serial.println("Bearer saved on this occasion! Make sure it's not in your production code!");
  }
#endif

  class GenericProvider
  {
  public:
    String OTA_SERVER;
    uint16_t OTA_PORT = 443;
    String OTA_CHECK_PATH;
    String OTA_BIN_PATH;
    String OTA_BEARER;
    bool OTA_USE_BEARER = false;
    String FIRMWARE_BIN_MATCH = "firmware.bin";
    bool FIRMWARE_WAS_BUILT_ON_PROVIDER = false; // Is set internally by our cloud scripts.

    String OTA_JSON_HEADER = "application/json";
    String OTA_DOTA_HEADER = "application/octet-stream";

    GenericProvider() {};

    String AssetEndpointConstructor(String asset_id)
    {
      return String(OTA_BIN_PATH) + asset_id;
    }

    bool UseBearerFromNVS()
    {
      Preferences bearer_store;
      bearer_store.begin("ota_hub-bearer");
      String token = bearer_store.getString("token", "fail_123!_");
      bearer_store.end();
      Serial.println("Token loaded!: " + token.substring(0, 10) + "...");
      if (token.equals("fail_123!_"))
        return false;

      OTA_BEARER = token;
      return true;
    }
  };
}