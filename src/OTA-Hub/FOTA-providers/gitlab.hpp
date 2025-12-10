#pragma once
#include "generic-provider.hpp"
#include "certs/gitlab-certs.hpp" // optional if you have GitLab CA

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
    GitlabProvider(String group_or_user, String project, bool use_bearer_from_nvs = false) : GenericProvider()
    {
      OTA_SERVER = "gitlab.com";
      OTA_PORT = 443;
      OTA_CHECK_PATH = "/api/v4/projects/" + urlEncode(group_or_user + "/" + project) + "/releases";
      OTA_BIN_PATH = "/api/v4/projects/" + urlEncode(group_or_user + "/" + project) + "/releases/";
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
    GitlabProvider(String group_or_user, String project, String firmware_match, bool use_bearer_from_nvs = false)
        : GitlabProvider(group_or_user, project, use_bearer_from_nvs)
    {
      FIRMWARE_BIN_MATCH = firmware_match;
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
