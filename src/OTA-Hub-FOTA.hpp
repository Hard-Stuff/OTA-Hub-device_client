#pragma once
#define HTTP_MAX_HEADERS 30 // GitHub sends ~28 headers back!

#include <Arduino.h>
SET_LOOP_TASK_STACK_SIZE(16 * 1024); // 16KB, GitHub responses are heavy

#ifndef OTA_BUF_SIZE
#define OTA_BUF_SIZE 2048
#endif

// libs
#include <Hard-Stuff-Http.hpp>
#include <Update.h>
#include <ArduinoJson.h>
#include <OTA-Hub/types.h>
#include <OTA-Hub-utils.hpp>
#include <OTA-Hub/FOTA-providers/generic-provider.hpp>

#ifndef OTA_VERSION
#define OTA_VERSION "local_development"
#endif

#ifndef OTAHUB_STDOUT
#define OTAHUB_STDOUT Serial.println
#endif

namespace OTAHub::FOTA
{
#pragma region WorkingVariabls
  HardStuffHttpClient *http_ota;
  Client *underlying_client;
  String asset_id;
  GenericProvider *ota_provider;
#pragma endregion

#pragma region SupportFunctions

  void confirmConnected()
  {
    http_ota->connected();
  }

  void printFirmwareDetails(Stream *print_stream = &Serial)
  {
    print_stream->println("------------------------");
    print_stream->println("Device MAC: " + getMacAddress());
    print_stream->println("Firmware Version: " + String(OTA_VERSION));
    print_stream->println("Firmware Compilation Date: " + String(__DATE__) + ", " + String(__TIME__));
    print_stream->println("------------------------");
  }

  void deinit()
  {
    if (http_ota != nullptr)
    {
      http_ota->stop();
      delete http_ota;
      http_ota = nullptr;
    }
  }

  void reinit(Client &set_underlying_client, const char *server, uint16_t port)
  {
    deinit();
    Serial.print("Server: ");
    Serial.print(server);
    Serial.print(", port: ");
    Serial.println(port);
    underlying_client = &set_underlying_client;
    http_ota = new HardStuffHttpClient(set_underlying_client, server, port);
  }

  void init(Client &set_underlying_client, GenericProvider &set_provider)
  {
    printFirmwareDetails();
    /*
      Here we need to retrieve the token from preferences
    */
    ota_provider = &set_provider;

#ifdef OTAHUB_BEARER_TOKEN
    OTAHub::FOTA::SaveBearerToNVS();
#endif

    if (ota_provider->OTA_USE_BEARER)
      if (!ota_provider->UseBearerFromNVS())
        Serial.println("Could not load Bearer token from NVS! This may not work going forwards.");

#ifdef OTAHUB_DEBUG
    Serial.print("OTA_SERVER:");
    Serial.println(ota_provider->OTA_SERVER);
    Serial.print("OTA_PORT:");
    Serial.println(ota_provider->OTA_PORT);
    Serial.print("OTA_CHECK_PATH:");
    Serial.println(ota_provider->OTA_CHECK_PATH);
    Serial.print("OTA_BEARER:");
    Serial.println(ota_provider->OTA_BEARER);
    Serial.print("OTA_JSON_HEADER:");
    Serial.println(ota_provider->OTA_JSON_HEADER);
    Serial.print("OTA_USE_BEARER:");
    Serial.println(ota_provider->OTA_USE_BEARER);
#endif

    reinit(set_underlying_client, ota_provider->OTA_SERVER.c_str(), ota_provider->OTA_PORT);
  }

#pragma endregion

#pragma region CoreFunctions

  /**
   * @brief Check your server (or GitHub) to see if an update is available
   *
   * @return UpdateObject that bundles all the info we'll need.
   */
  UpdateObject isUpdateAvailable()
  {
    UpdateObject return_object;
    return_object.condition = NO_UPDATE;

    // Get the response from the server
    HardStuffHttpRequest request;
    request.addHeader("accept", ota_provider->OTA_JSON_HEADER);
    if (ota_provider->OTA_USE_BEARER)
      request.addHeader("authorization", "Bearer " + String(ota_provider->OTA_BEARER)); // Used only in private repos. See the docs.

    HardStuffHttpResponse response = http_ota->getFromHTTPServer(ota_provider->OTA_CHECK_PATH, &request);

    if (response.success())
    {
      // Compile into a JSON doc
#if ARDUINOJSON_VERSION_MAJOR >= 7
      JsonDocument release_response;
#else
      DynamicJsonDocument release_response(8129);
#endif
      deserializeJson(release_response, response.body);

      if (!ota_provider->AssetEndpointCheck(release_response))
      {
        Serial.println("The latest release contains no assets and/or metadata. We can't continue...");
        return return_object;
      }

      ota_provider->AssetEndpointFinder(release_response, return_object);

#if ARDUINOJSON_VERSION_MAJOR < 7
      release_response.clear();
#endif
      return return_object;
    }

    Serial.print("Failed to connect to " + ota_provider->OTA_SERVER + ": ");
    switch (response.status_code)
    {
    case 401:
      Serial.print("401 - Unauthorized.");
      break;

    case 404:
      Serial.print(
          "404 - Not found" + String(
                                  !ota_provider->OTA_USE_BEARER
                                      ? ". Do you need an auth token? (which hasn't been set!), or are you sure it exists?"
                                      : " despite your auth token.. are you sure it exists?"));
      break;
    default:
      Serial.print(String(response.status_code) + ". Not sure why though...");
    }
    Serial.println(" Check your OTA_... #defines.");
    return return_object;
  }

  size_t writeStream(uint32_t contentLength)
  {
    uint8_t buff[OTA_BUF_SIZE];
    size_t totalWritten = 0;
    unsigned long lastRead = millis();

    while (totalWritten < contentLength)
    {
      int avail = http_ota->available();

      if (avail > 0)
      {
        int len = http_ota->read(buff, sizeof(buff));

        if (len < 0)
        {
          Serial.println("Read error");
          break;
        }

        if (len == 0)
        {
          // harmless — TLS buffer might be refilling
          delay(1);
          continue;
        }

        size_t w = Update.write(buff, len);

        if (w != (size_t)len)
        {
          Serial.printf("Flash write mismatch! wrote %u of %d\n", w, len);
          break;
        }

        totalWritten += w;
        lastRead = millis();
      }
      else
      {
        if (millis() - lastRead > 3000)
        {
          Serial.println("Timeout waiting for data");
          break;
        }

        delay(1); // keeps WiFi alive
        yield();  // essential for OTA stability
      }
    }

    Serial.printf("Streamed: %u / %u\n", totalWritten, contentLength);
    return totalWritten;
  }

  /**
   * @brief Download and perform an update based on the details provided in the UpdateObject file.
   *
   * @param details You'll get this from `isUpdateAvailable`
   * @param restart You can stop the updater from automatically restarting the board, say if you need to wind things down a bit...
   * @return InstallCondition Was it a success?
   */
  InstallCondition performUpdate(UpdateObject *details, bool restart = true)
  {
    Serial.println("Starting OTA Process...");
    Serial.println("Target: " + details->firmware_asset_endpoint);

    // Headers
    HardStuffHttpRequest request;
    request.addHeader("accept", ota_provider->OTA_DOTA_HEADER);
    if (ota_provider->OTA_USE_BEARER)
      request.addHeader("authorization", "Bearer " + String(ota_provider->OTA_BEARER)); // Used only in private repos. See the docs.

    HardStuffHttpResponse response = http_ota->getFromHTTPServer(details->firmware_asset_endpoint, &request, true);

    if (response.success())
    {
      int contentLength = response.content_length;

      if (contentLength <= 0)
      {
        Serial.println("Error: Content-Length is invalid (0).");
        http_ota->stop();
        return FAILED_TO_DOWNLOAD;
      }

      Serial.println("Download stream ready. Size: " + String(contentLength) + " bytes.");
      Update.onProgress([](size_t written, size_t total)
                        {
    static int lastReported = -1; // persist across calls
    int percent = (written * 100) / total;

    // Only print every 10%
    if (percent / 10 != lastReported / 10)
    {
        lastReported = percent;
        Serial.printf("Progress: %u / %u (%u%%)\n", written, total, percent);
    } });

      // 4. Begin OTA Flash
      if (!Update.begin(contentLength, U_FLASH))
      {
        Serial.printf("Not enough space! Error: %d\n", Update.getError());
        return FAILED_TO_DOWNLOAD;
      }
      else
      {
        // Stream data directly from the open socket to the flash partition
        size_t written = writeStream(contentLength);

        if (written == contentLength)
        {
          Serial.println("Written : " + String(written) + "/" + String(contentLength));

          if (Update.end())
          {
            if (Update.isFinished())
            {
              Serial.println("OTA done!");
              if (restart)
              {
                Serial.println("Reboot...");
                ESP.restart();
              }
              return SUCCESS;
            }
            else
            {
              Serial.println("Update finished but validation failed.");
            }
          }
          else
          {
            Serial.printf("Update.end() failed. Error: %d\n", Update.getError());
          }
        }
        else
        {
          Serial.printf("Write incomplete: %u / %u. Aborting.\n", written, contentLength);
          Update.abort();
        }
      }
    }
    else
    {
      Serial.print("Download failed. HTTP Code: ");
      Serial.println(response.status_code);
      if (response.status_code == HTTP_ERROR_TIMED_OUT)
        Serial.println("Reason: Connection Timed Out or Too Many Redirects.");
    }

    http_ota->stop();
    return FAILED_TO_DOWNLOAD;
  }
#pragma endregion
}