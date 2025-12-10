#pragma once
#define HTTP_MAX_HEADERS 30 // GitHub sends ~28 headers back!

#include <Arduino.h>
SET_LOOP_TASK_STACK_SIZE(16 * 1024); // 16KB, GitHub responses are heavy

// libs
#include <Hard-Stuff-Http.hpp>
#include <Update.h>
#include <ArduinoJson.h>
#include <OTA-Hub-utils.hpp>
#include <OTA-Hub/FOTA-providers/generic-provider.hpp>

#ifndef OTA_VERSION
#define OTA_VERSION "local_development"
#endif

namespace OTAHub::FOTA
{
#pragma region WorkingVariabls
  HardStuffHttpClient *http_ota;
  Client *underlying_client;
  String asset_id;
  GenericProvider *ota_provider;
#pragma endregion

#pragma region UsefulStructs
  /**
   * @brief What is the condition of the update relative to the current installed firmware?
   */
  enum UpdateCondition
  {
    NO_UPDATE,     // The proposed release is the same name and same age as this one (i.e. they're the same)
    OLD_DIFFERENT, // The proposed release is different to what we've got here (but it's older)
    NEW_SAME,      // The proposed release is newer but has the same name as this one (are you versioning correctly?)
    NEW_DIFFERENT  // The proposed update is both newer and has a different name (so is likely to be a legitimate update)
  };

  /**
   * @brief What is the condition of the install?
   */
  enum InstallCondition
  {
    FAILED_TO_DOWNLOAD, // For whatever reason, we failed to download the update
    REDIRECT_REQUIRED,  // We'll need to follow a redirect to download the firmware.bin file. Don't forget to set to the new ca_cert!
    SUCCESS             // Success! You'll likely only see me if you've asked the installer to not restart the ESP32.
  };

  /**
   * @brief Everything necesary related to a firmware release
   */
  struct UpdateObject
  {
    UpdateCondition condition;
    String name;
    String tag_name;
    time_t published_at;
    String firmware_asset_id;
    String firmware_asset_endpoint;
    String redirect_server;

    void print(Stream *print_stream = &Serial)
    {
      const char *condition_strings[] = {
          "NO_UPDATE",
          "OLD_DIFFERENT",
          "NEW_DIFFERENT",
          "NEW_SAME"};

      // Print condition
      print_stream->println("------------------------");
      print_stream->println("Condition: " + String(condition_strings[condition]));
      print_stream->println("name: " + name);
      print_stream->println("tag_name: " + tag_name);
      print_stream->println("published_at: " + http_ota->formatTimeISO8601(published_at));
      print_stream->println("firmware_asset_id: " + String(firmware_asset_id));
      print_stream->println("firmware_asset_endpoint: " + String(firmware_asset_endpoint));
      print_stream->println("------------------------");
    }
  };
#pragma endregion

#pragma region SupportFunctions

  // Initial define of function
  InstallCondition continueRedirect(UpdateObject *details, bool restart = true);

  void confirmConnected()
  {
    if (http_ota->connected())
    {
    }
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

    Serial.print("OTA_SERVER:");
    Serial.println(ota_provider->OTA_SERVER);
    Serial.print("OTA_PORT:");
    Serial.println(ota_provider->OTA_PORT);
    Serial.print("OTA_CHECK_PATH:");
    Serial.println(ota_provider->OTA_CHECK_PATH);
    Serial.print("OTA_BIN_PATH:");
    Serial.println(ota_provider->OTA_BIN_PATH);
    Serial.print("OTA_BEARER:");
    Serial.println(ota_provider->OTA_BEARER);
    Serial.print("OTA_JSON_HEADER:");
    Serial.println(ota_provider->OTA_JSON_HEADER);
    Serial.print("OTA_USE_BEARER:");
    Serial.println(ota_provider->OTA_USE_BEARER);

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
      if (
#if ARDUINOJSON_VERSION_MAJOR >= 7
          release_response["name"].isNull() ||
          release_response["published_at"].isNull() ||
          release_response["assets"].isNull()
#else
          !release_response.containsKey("name") ||
          !release_response.containsKey("published_at") ||
          !release_response.containsKey("assets")
#endif
      )
      {
        Serial.println("The latest release contains no assets and/or metadata. We can't continue...");
        return return_object;
      }

      return_object.name = release_response["name"].as<String>();
      return_object.tag_name = release_response["tag_name"].as<String>();
      return_object.published_at = http_ota->formatTimeFromISO8601(release_response["published_at"].as<String>());

      // Evaluate comparison based on metadata
      bool update_is_different = release_response["name"].as<String>().compareTo(OTA_VERSION) != 0;
      bool update_is_newer = release_response["published_at"].as<time_t>() > cvtDate();

      JsonArray asset_array = release_response["assets"].as<JsonArray>();
      for (JsonVariant v : asset_array)
      {
        if (v["name"].as<String>().compareTo(ota_provider->FIRMWARE_BIN_MATCH) == 0)
        {
          return_object.firmware_asset_id = v["id"].as<String>();
          return_object.condition = update_is_different ? (update_is_newer ? NEW_DIFFERENT : OLD_DIFFERENT) : (update_is_newer ? NEW_SAME : NO_UPDATE);
          return_object.firmware_asset_endpoint = ota_provider->AssetEndpointConstructor(return_object.firmware_asset_id);
          return return_object;
        }
      }
      Serial.println("The latest release contains no firmware asset. We can't continue...");
#if ARDUINOJSON_VERSION_MAJOR < 7
      release_response.clear();
#endif
      return return_object;
    }

    Serial.print("Failed to connect to GitHub. ");
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

  /**
   * @brief Download and perform an update based on the details provided in the UpdateObject file.
   *
   * @param details You'll get this from `isUpdateAvailable`
   * @param restart You can stop the updater from automatically restarting the board, say if you need to wind things down a bit...
   * @return InstallCondition Was it a success?
   */
  InstallCondition performUpdate(UpdateObject *details, bool follow_redirects = true, bool restart = true)
  {
    Serial.println("Fetching update from: " + (details->redirect_server.isEmpty() ? String(ota_provider->OTA_SERVER) : details->redirect_server) + details->firmware_asset_endpoint);

    HardStuffHttpRequest request;
    // Headers
    request.addHeader("accept", ota_provider->OTA_DOTA_HEADER);
    if (ota_provider->OTA_USE_BEARER)
      request.addHeader("authorization", "Bearer " + String(ota_provider->OTA_BEARER)); // Used only in private repos. See the docs.

    // On GitHub this will likely return a 302 with a "location" header:
    HardStuffHttpResponse response = http_ota->getFromHTTPServer(details->firmware_asset_endpoint, &request, true);

    if (response.status_code == 302)
    {
      // Do redirect logic
      // Extract URL from "Location"
      String URL = "";
      for (int i = 0; i < response.header_count; i++)
      {
        if (response.headers[i].key.compareTo("Location") == 0)
        {
          URL = response.headers[i].value;
          break;
        }
      }
      if (URL.isEmpty())
      {
        Serial.println("Redirection URL extraction error...");
        Serial.println("We can't continue.");
        return FAILED_TO_DOWNLOAD;
      }
      // Get .com/ (or other part);
      URL.replace("https://", "");
      URL.replace("http://", "");
      int slash_index = URL.indexOf("/");

      http_ota->stop();
      delay(250);
      details->redirect_server = URL.substring(0, slash_index);
      details->firmware_asset_endpoint = URL.substring(slash_index);
      if (follow_redirects)
      {
        Serial.println("Redirect required, handling internally...");
        return continueRedirect(details, restart);
      }
      else
      {
        return REDIRECT_REQUIRED;
      }
    }

    if (response.status_code >= 200 && response.status_code < 300)
    {
      // we can download as normal
      Serial.println(String(ota_provider->FIRMWARE_BIN_MATCH) + " found. Checking validity.");
      int contentLength;
      bool isValidContentType;

      for (int i_header = 0; i_header < response.header_count; i_header++)
      {
        if (response.headers[i_header].key.compareTo("Content-Length") == 0)
        {
          contentLength = response.headers[i_header].value.toInt();
        }
        if (response.headers[i_header].key.compareTo("Content-Type") == 0)
        {
          String contentType = response.headers[i_header].value;
          isValidContentType =
              contentType == "application/octet-stream" || contentType == "application/macbinary";
        }
      }

      if (contentLength && isValidContentType)
      {
        Serial.println(String(ota_provider->FIRMWARE_BIN_MATCH) + " is good. Beginning the OTA update, this may take a while...");
        if (Update.begin(contentLength))
        {
          Update.writeStream(*http_ota);
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
              http_ota->stop();
              return SUCCESS;
            }
          }
        }

        Serial.println("------------------------------ERROR------------------------------");
        Serial.printf("    ERROR CODE: %d", Update.getError());
        Serial.println("-----------------------------------------------------------------");
      }
      else
      {
        Serial.println("Content isn't a valid *.bin download.");
      }
    }
    else
    {
      Serial.println("There was no content in the response");
      response.print(); // Log the errors
    }
    http_ota->stop();
    return FAILED_TO_DOWNLOAD;
  }

  /**
   * @brief Continue with an update (likely modified) following a 302 redirect.
   * Behaves similar to performUpdate, but is used after defining new SSL certs as needed.
   * @return InstallCondition
   */
  InstallCondition continueRedirect(UpdateObject *details, bool restart)
  {
    reinit(*underlying_client, details->redirect_server.c_str(), ota_provider->OTA_PORT);
    return performUpdate(details, false, restart);
  }
#pragma endregion
}