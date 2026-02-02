#include <Arduino.h>

// OTA Hub via GitLab
#include <OTA-Hub.hpp>
#include <OTA-Hub/FOTA-providers/gitlab.hpp>

#define SIM7600_APN "Three"             // Your SIM's APN, e.g. Vodafone, TMobile, etc.
#include <Hard-Stuff-SIM7600.hpp>       // Add "hard-stuff/SIM7600@^0.0.2" to your platformio.ini lib_deps (or use TinyGSM)
SIM7600::ClientSecure secure_client(0); // There are 2 dedicated secure clients and 8 total clients on the SIM7600.

OTAHub::FOTA::GitlabProvider provider(
    13149231, // An example GitLab (private) repo id.
    true      // It's going to look in NVS for a GitLab Personal Access Token.
);

void setup()
{
  // Initialise our board
  Serial.begin(115200);
  Serial.println("Started...");

  // Initialise OTA
  secure_client.setCACert(OTAGH_CA_CERT); // Set the api.github.cm SSL cert on the SIM7600 Client
  SIM7600::init();
  OTAHub::FOTA::init(secure_client, provider);

  // Check OTA for updates
  OTAHub::FOTA::UpdateObject details = OTAHub::FOTA::isUpdateAvailable();
  details.print(); // Super useful for debugging!

  if (OTA::NEW_DIFFERENT == details.condition) // Only update if the update is both new and a different version name
  {
    Serial.println("An update is available!");
    // Perform OTA update - will auto restart
    if (OTAHub::FOTA::performUpdate(&details) != OTAHub::FOTA::SUCCESS)
    {
      // Something is wrong...
    }
  }
  else
  {
    Serial.println("No new update available. Continuing...");
  }
  Serial.print("Loop");
}

void loop()
{
  delay(5000);
  Serial.print("edy loop");
}