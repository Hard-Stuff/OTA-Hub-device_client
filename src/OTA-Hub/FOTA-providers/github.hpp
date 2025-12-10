#pragma once
#include "generic-provider.hpp"
#include "certs/github-certs.hpp"

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