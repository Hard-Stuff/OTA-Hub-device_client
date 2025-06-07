#include <Arduino.h>

// OTA Hub via GitHub
#define OTAGH_OWNER_NAME "YOUR GITHUB ID"
#define OTAGH_REPO_NAME "YOUR REPO NAME"
#define OTAGH_BEARER "YOUR PRIVATE REPO TOKEN" // Follow the docs if using a private repo. Remove if repo is public.
#include <OTA-Hub.hpp>

#define SIM7600_APN "Three"             // Your SIM's APN
#include <Hard-Stuff-SIM7600.hpp>       // Add "hard-stuff/SIM7600@^0.0.2" to your platformio.ini lib_deps
SIM7600::ClientSecure secure_client(0); // There are 2 dedicated secure clients and 8 total clients on the SIM7600.

void setup()
{
    // Initialise our board
    Serial.begin(115200);
    Serial.println("Started...");

    // Initialise OTA
    secure_client.setCACert(OTAGH_CA_CERT); // Set the api.github.cm SSL cert on the SIM7600 Client
    SIM7600::init();
    OTA::init(secure_client);

    // Check OTA for updates
    OTA::UpdateObject details = OTA::isUpdateAvailable();
    details.print();                             // Super useful for debugging!
    if (OTA::NEW_DIFFERENT == details.condition) // Only update if the update is both new and a different version name
    {
        // Perform OTA update
        if (OTA::performUpdate(&details) == OTA::SUCCESS)
        {
            // .. success! It'll restart by default, or you can do other things here...
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