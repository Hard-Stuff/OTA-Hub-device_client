#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <OTA-Hub/types.h>

namespace OTAHub::FOTA
{
#define OTAHUB_BEARER_NVS_STORE "ota_hub-bearer"
#define OTAHUB_BEARER_NVS_STORE_TOKEN "token"
#ifdef OTAHUB_BEARER_TOKEN

#ifndef OTA_VERSION
#define OTA_VERSION "local_development"
#endif

#pragma message("OTAHUB_BEARER_TOKEN is defined - bearer token will be saved to NVS at runtime.")
  void SaveBearerToNVS()
  {
    Preferences bearer_store;
    bearer_store.begin(OTAHUB_BEARER_NVS_STORE);
    bearer_store.putString(OTAHUB_BEARER_NVS_STORE_TOKEN, OTAHUB_BEARER_TOKEN);
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

    /**
     * Overload this with your nominal behaviour.
     */
    virtual void AssetEndpointFinder(JsonDocument &release_response, UpdateObject &return_object)
    {
    }

    virtual bool AssetEndpointCheck(JsonDocument release_response) { return true; }

    bool UseBearerFromNVS()
    {
      Preferences bearer_store;
      bearer_store.begin(OTAHUB_BEARER_NVS_STORE);
      String token = bearer_store.getString(OTAHUB_BEARER_NVS_STORE_TOKEN, "fail_123!_");
      bearer_store.end();
      if (token.equals("fail_123!_"))
        return false;

      Serial.println("Token loaded!: " + token.substring(0, 10) + "...");
      OTA_BEARER = token;
      return true;
    }
  };
}