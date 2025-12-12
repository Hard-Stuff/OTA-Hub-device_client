#pragma once

#include <Arduino.h>

namespace OTAHub::FOTA
{
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
      print_stream->println("published_at: " + published_at);
      print_stream->println("firmware_asset_id: " + String(firmware_asset_id));
      print_stream->println("firmware_asset_endpoint: " + String(firmware_asset_endpoint));
      print_stream->println("------------------------");
    }
  };
}
