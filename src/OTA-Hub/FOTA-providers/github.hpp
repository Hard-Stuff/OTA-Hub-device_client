#pragma once
#include <OTA-Hub/types.h>
#include <OTA-Hub-utils.hpp>
#include "generic-provider.hpp"
#include "certs/github-certs.hpp"
#include <ArduinoJson.h>
#include <Hard-Stuff-Http/utils.hpp>

namespace OTAHub::FOTA
{
  class GithubProvider : public GenericProvider
  {
  public:
    /**
     * @brief Constructor for GitLab OTA provider
     *
     * @param organisation The Organisation on GitHub, e.g. Hard-Stuff
     * @param repository The Repository name, e.g. e.g. OTA-Hub-diy-example_project
     * @param use_bearer_from_nvs Whether to load a private token from NVS
     */
    GithubProvider(String organisation, String repository, bool use_bearer_from_nvs = false) : GenericProvider()
    {
      OTA_SERVER = "api.github.com";
      OTA_PORT = 443;
      OTA_CHECK_PATH = "/repos/" + organisation + "/" + repository + "/releases/latest";
      OTA_BIN_PATH = "/repos/" + organisation + "/" + repository + "/releases/assets/";
      OTA_BEARER = "";
      OTA_JSON_HEADER = "application/vnd.github+json";
      OTA_USE_BEARER = use_bearer_from_nvs;
#ifdef OTA_BUILT_ON_GITHUB
      FIRMWARE_WAS_BUILT_ON_PROVIDER = true; // Is set internally by our cloud scripts.
#endif
    };

    bool AssetEndpointCheck(JsonDocument release_response) override
    {
#if ARDUINOJSON_VERSION_MAJOR >= 7
      return !(release_response["name"].isNull() ||
               release_response["published_at"].isNull() ||
               release_response["assets"].isNull());
#else
      return (
          !release_response.containsKey("name") ||
          !release_response.containsKey("published_at") ||
          !release_response.containsKey("assets"));
#endif
    }

    void AssetEndpointFinder(JsonDocument &release_response, UpdateObject &return_object) override
    {
      return_object.name = release_response["name"].as<String>();
      return_object.tag_name = release_response["tag_name"].as<String>();
      return_object.published_at = formatTimeFromISO8601(release_response["published_at"].as<String>());

      // Evaluate comparison based on metadata
      bool update_is_different = release_response["name"].as<String>().compareTo(OTA_VERSION) != 0;
      bool update_is_newer = release_response["published_at"].as<time_t>() > cvtDate();

      JsonArray asset_array = release_response["assets"].as<JsonArray>();
      for (JsonVariant v : asset_array)
      {
        if (v["name"].as<String>().compareTo(FIRMWARE_BIN_MATCH) == 0)
        {
          return_object.firmware_asset_id = v["id"].as<String>();
          return_object.condition = update_is_different ? (update_is_newer ? NEW_DIFFERENT : OLD_DIFFERENT) : (update_is_newer ? NEW_SAME : NO_UPDATE);
          String url =  v["url"].as<String>();
          return_object.firmware_asset_endpoint = extractEndpointFromURL(url); 
          break;
        }
      }
    }

    /**
     * @brief Constructor with firmware match string, default is firmware.bin
     */
    GithubProvider(String organisation, String repository, String firmware_match, bool use_bearer_from_nvs = false)
        : GithubProvider(organisation, repository, use_bearer_from_nvs) // proper delegating constructor
    {
      FIRMWARE_BIN_MATCH = firmware_match;
    }
  };

}