#pragma once
#include <OTA-Hub/types.h>
#include <OTA-Hub-utils.hpp>
#include "generic-provider.hpp"
#include "certs/gitlab-certs.hpp" // optional if you have GitLab CA
#include <ArduinoJson.h>
#include <Hard-Stuff-Http/utils.hpp>

namespace OTAHub::FOTA
{
  class GitlabProvider : public GenericProvider
  {
  public:
    /**
     * @brief Constructor for GitLab OTA provider
     *
     * @param group_or_user The GitLab group or username, e.g. Hard-Stuff
     * @param project The GitLab project name
     * @param use_bearer_from_nvs Whether to load a private token from NVS
     */
    GitlabProvider(u_int64_t project_id, bool use_bearer_from_nvs = false) : GenericProvider()
    {
      OTA_SERVER = "gitlab.com";
      OTA_PORT = 443;

      OTA_CHECK_PATH = "/api/v4/projects/" + String(project_id) + "/releases/permalink/latest";
      OTA_BEARER = "";
      OTA_JSON_HEADER = "application/json";
      OTA_USE_BEARER = use_bearer_from_nvs;
#ifdef OTA_BUILT_ON_GITLAB
      FIRMWARE_WAS_BUILT_ON_PROVIDER = true; // Is set internally by our cloud scripts.
#endif
    };

    /**
     * @brief Constructor with firmware match string, default is firmware.bin
     */
    GitlabProvider(u_int64_t project_id, String firmware_match, bool use_bearer_from_nvs = false)
        : GitlabProvider(project_id, use_bearer_from_nvs)
    {
      FIRMWARE_BIN_MATCH = firmware_match;
    }

    bool AssetEndpointCheck(JsonDocument release_response) override
    {
#if ARDUINOJSON_VERSION_MAJOR >= 7
      return !(release_response["name"].isNull() ||
               release_response["released_at"].isNull() ||
               release_response["assets"].isNull());
#else
      return (
          !release_response.containsKey("name") ||
          !release_response.containsKey("released_at") ||
          !release_response.containsKey("assets"));
#endif
    }

    void AssetEndpointFinder(JsonDocument &release_response, UpdateObject &return_object) override
    {
      return_object.name = release_response["name"].as<String>();
      return_object.tag_name = release_response["tag_name"].as<String>();
      return_object.published_at = formatTimeFromISO8601(release_response["released_at"].as<String>());

      // Evaluate comparison based on metadata
      bool update_is_different = release_response["name"].as<String>().compareTo(OTA_VERSION) != 0;
      bool update_is_newer = release_response["released_at"].as<time_t>() > cvtDate();

      JsonObject assets_group = release_response["assets"].as<JsonObject>();
      JsonArray links_array = assets_group["links"].as<JsonArray>();
      for (JsonVariant v : links_array)
      {
        if (v["name"].as<String>().compareTo(FIRMWARE_BIN_MATCH) == 0)
        {
          return_object.firmware_asset_id = v["id"].as<String>();
          return_object.condition = update_is_different ? (update_is_newer ? NEW_DIFFERENT : OLD_DIFFERENT) : (update_is_newer ? NEW_SAME : NO_UPDATE);
          String url = v["url"].as<String>();
          return_object.firmware_asset_endpoint = extractEndpointFromURL(url);
          break;
        }
      }
    }

  private:
    /**
     * @brief URL encode a string (GitLab project paths need URL encoding)
     */
    String urlEncode(const String &str)
    {
      String encoded = "";
      char c;
      char code0;
      char code1;
      for (int i = 0; i < str.length(); i++)
      {
        c = str.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
          encoded += c;
        }
        else
        {
          code1 = (c & 0xf) + '0';
          if ((c & 0xf) > 9)
            code1 = (c & 0xf) - 10 + 'A';
          code0 = ((c >> 4) & 0xf) + '0';
          if (((c >> 4) & 0xf) > 9)
            code0 = ((c >> 4) & 0xf) - 10 + 'A';
          encoded += '%';
          encoded += code0;
          encoded += code1;
        }
      }
      return encoded;
    }
  };
}
