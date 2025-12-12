# **OTA Hub (by Hard Stuff)** - Over the Air Firmware Updates directly from GitHub, GitLab, and more!

**OTA Hub** is a solution to enable building and hosting your firmware updates on Git (Hub or Lab) and accessing them simply on your ESP devices. That's this repo, and it's entirely open source.

**OTA Hub Cloud** _(coming soon)_ is a cloud software that builds on that by enabling per-device provisioning, and data endpoints to plug into your stack. Think AWS IoT Core but massively simplified - great for hobbyists, prototyping, and startups. OTA Hub Cloud comes in both free and paid tiers.

**What sucks about current systems**

- OTA has long been a sore point in hobbyist and early-stage prototyping (e.g. startups), because while AWS and Blynk etc. offer OTA solutions, they all require so much set up, and crucially so much commitment that it hardle seems worth it.
- Combine that with the weird insistence on drag and dropping `.bin` files into file upload ports, or constantly replacing the file in a dedicated S3 bucket - it becomes a mess bound to go wrong (and impossible to actually trace).
  
**Why this library rocks 🤘 (for hobbyists & prototypes)**

1. You can use GitHub/GitLab as your build engine and storage solution for your `.bin` firmware files, obviously in the same place as your actual code.
2. That means it's cheaper (free), easier, more obvious, and simpler to track what code is where.
3. You're not married to any one platform/solution - easy to drop in, but easy to drop out and upscale when needed!
4. This works for Public and your Private Git repos.
5. It's incredibly flexible - on top of GitHub, GitLab, and OTAHub Cloud, you can leverage this library with many other build providers, and we're implementing more soon!
6. It works on any abstract HTTP client, meaning `WiFi`, `WiFiClientSecure`, `TinyGSM` (4G/5G), even `LTEm/NBIoT modems`.


## Git release usage

The OTA process can now be simplified to:
1. Have your firmware code hosted on GitHub/GitLab - we build our firmware in `platformio` projects.
2. Use Git's CI/CD to build that firmware into releasable `.bin` files hosted there - we have drop in examples here.
3. Your firmware would have OTA Hub Device Client running, and you check for a new release e.g. on boot. This finds the latest release, and based on your logic (see below) downloads and flashes it.
4. _That is it.. it's that simple!_

**OTA Hub** is designed to do one thing, and one thing only:

    Deliver Over-the-Air updates onto your ESP32/embedded devices directly from your code releases in an obvious, clean, light-weight way.

    It is designed to do this on an abstract Client - this means it works equally well over WiFi (and WiFiClientSecure) as it does over 4G/5G clients such as TinyGSM.

**OTA Hub** is for the hobbyists and small teams, directly grabbing release files from GitHub, involving as minimal setup as possible. It's completely open-source, and of course, free!

**OTA Hub Cloud** comes both in free and professional tiers, providing you with a dashboard to have finer control over your release deployment, fleet management, greater flexibility, and even less setup! Find out more at [ota-hub.com/pro](ota-hub.com/pro).

## Usage

_You must first have CI/CD set up on your firmware repo of choice. Follow [the docs](https://github.com/Hard-Stuff/OTA-Hub-diy-example_project) for a simple copy-paste guide on how to do this -> you can use the above example project as a robust template._

The flow logic for this entire OTA library is super simple:

1. **Check for updates** - It first checks on your GitHub repo for the latest release of your firmware. GitHub reports back the `name` and `published_at` timestamp of the latest **release**.
2. **Perform the update** - If given the information in step 1 compared to the current installation you want to perform the update: automatically download and install the `firmware.bin` file onto the device.
3. **Follow the redirect** - Because GitHub hosts the release data on `api.github.com` but the `firmware.bin` asset on `objects.githubusercontent.com`, we may have to manually follow the redirect **after** updating the SSL CA Cert. _This suck, but it's how GitHub has built it 🤷‍♂️._

### Basic Example

```cpp
#include <Arduino.h>

// OTA Hub via GitHub
#include <OTA-Hub.hpp>
#include <OTA-Hub/FOTA-providers/github.hpp> // Only needed for github

#include <WiFiClientSecure.h>

WiFiClientSecure wifi_client;
OTAHub::FOTA::GithubProvider provider(
    "Hard-Stuff",
    "OTA-Hub-examples"); // Assumed a public repo

void setup()
{
  Serial.begin(115200);
  Serial.println("Started...");

  WiFi.begin("Your SSID", "Your PASS");
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi failure");
    ESP.restart();
  }

  // Initialise OTA
  wifi_client.setCACert(OTAHub::certs::GITHUB_CA); // Needed externally because you can use any (un)secure client!
  OTAHub::FOTA::init(wifi_client, provider);

  // Check OTA for updates
  OTAHub::FOTA::UpdateObject details = OTAHub::FOTA::isUpdateAvailable();
  details.print(); // Optionally print to Serial.

  Serial.println(OTAHub::FOTA::ota_provider->FIRMWARE_WAS_BUILT_ON_PROVIDER
                     ? "This was built on Git."
                     : "This was built locally.");

  if (OTAHub::FOTA::NO_UPDATE != details.condition)
  {
    Serial.println("An update is available!");
    // Perform OTA update - will auto restart
    if (OTAHub::FOTA::performUpdate(&details) != OTAHub::FOTA::SUCCESS)
      // Something is wrong... Do something
      Serial.println("Failed to download / install. Bother...")
  }
  else
    Serial.println("No new update available. Continuing...");

  // As normal
}

void loop()
{
  // As normal
}
```

## Providers

### GitHub

```cpp
#include <OTA-Hub/FOTA-providers/github.hpp>
...
OTAHub::FOTA::GithubProvider provider(
    "Hard-Stuff",               // The organisation / user
    "private_repo",             // The repo name, in this case this is a private repo
    "custom_firmware_name.bin", // The custom firmware string to match - useful if you have multiple firmware builds
    true                        // Needed to load the GitHub bearer token (only for private repo) from NVS.
    );
```

### GitLab

```cpp
#include <OTA-Hub/FOTA-providers/gitlab.hpp>
...
OTAHub::FOTA::GitlabProvider provider(
    123...123,                       // The project id
    "a_different_firmware_name.bin", // as above..
    true                             // as above.. (only for private repo)
    );
```

### Custom (dev your own!)

- See our guide on [CUSTOM-PROVIDERS.md](./CUSTOM-PROVIDERS.md) _(docs upgrade coming soon!)_

_\* Note that our default examples are for SSL-enabled connections, as GitHub requires a secure connection. As this is open-source, you can of course use your own storage buckets APIs for insecure connections etc._

## Private repositories (you will need to make a Personal Access Token or Fine Grain Access Token)

OTA Hub DIY works with both your public and private repositories, pulling release files (that are automatically compiled) directly from GitHub. If using a private repository, we need a Personal (or Fine Grain) Access Token (PAT) to represent you so devices can access your secure accoutn. [You can generate your PATs here](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens).

- Then, once created, you can dump the token in a `.token` file and run _(example for **Windows**)_: 
  ```powershell
  # windows
  $env:PLATFORMIO_BUILD_FLAGS = "-DOTAHUB_BEARER_TOKEN=\`"$((Get-Content .token -Raw).Trim())\`""
  ```
  ```bash
  # linux
  token=$(cat .token)
  export PLATFORMIO_BUILD_FLAGS="-DOTAHUB_BEARER_TOKEN=\\\"$token\\\""
  ```
  or you can run:
  ```powershell
  # windows
  $env:PLATFORMIO_BUILD_FLAGS='-DOTAHUB_BEARER_TOKEN=\"PUTYOURTOKENHERE\"';
  ```
  ```bash
  # linux
  export PLATFORMIO_BUILD_FLAGS='-DOTAHUB_BEARER_TOKEN=\"PUTYOURTOKENHERE\"'
  ```
  Then run `pio run -t upload` ONCE with the token, which writes the firmware, whereby on boot this token is stored to NVS on boot. All future builds should not need the token as we then retrieve it from NVS.


See our [4G example](./examples/SIM7600/) for a SIM7600 HTTPS implementation of this.


### Other functions to be aware of

-  `details.condition` is an UpdateCondition type, it can be:
    - `NO_UPDATE`,     // The proposed release is the same name and same age as this one (i.e. they're the same)
    - `OLD_DIFFERENT`, // The proposed release is different to what we've got here (but it's older)
    - `NEW_SAME`,      // The proposed release is newer but has the same name as this one (are you versioning correctly?)
    - `NEW_DIFFERENT`  // The proposed update is both newer and has a different name (so is likely to be a legitimate update)
    - This also means that if you flash locally and there is a release on GitHub already, that you'll get an "OLD_DIFFERENT" because you've flashed a firmware that isn't the latest release on GitHub.

### Dependencies

-   arduino-libraries/ArduinoHttpClient
-   paulstoffregen/Time
-    [hard-stuff/Http](https://registry.platformio.org/libraries/hard-stuff/Http)

### Note on CA Certs

- We've bundled a few GitHub and GitLab CA certs together to cover both's various HTTP reroutings.
- **Certificates expire!** They tend to last a while, these ones last until 2030, but that's something to be aware of. Once either certificate has expired your devices will not be able to perform OTA (until flashed with new certs) - this is something we're going to attempt to future-proof going forwards.
- Plus, as we've experienced recently, certificates that are in date [might just stop working](https://news.ycombinator.com/item?id=35295216). In which case it's not a bad idea to either have a fallback option (we recommend [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) for local flashing) or to watch this space for our future proofing.
- You can always set your own certs (should the default ones not work) via `wifi_client.setCACert(NEW CA CERT)` and `wifi_client.setCACert(NEW CA CERT)`.
- Or you can set `wifi_client.setInsecure()` and remove the `setCACerts...`. This means that the ESP32 will not validate if the api.github.com and objects.githubusercontent.com have the correct CA Certs, so could technically open up security issues (although for hobbyist / non-critical projects this should be fine). 

## Compatibility

This library has been tested on the ESP32S3 with both the internal WiFi functionality and a [SIMCOM SIM7600G](https://github.com/Hard-Stuff/TinyGSM), both with HTTP and HTTPS connections, both with GitHub and GitLab, and both on public and private repositories.

We are looking for people to support us in testing more boards, other connectivity functionalities, and making **OTA Hub Pro** even more useful.

## Contribution
We're looking for people to work with us further on this - you can get started with issue reports or merge rquests, or you can contact us at [ota-hub@hard-stuff.com](mailto:ota-hub@hard-stuff.com).

## Hard Stuff

Hard Stuff is a hardware prototyping agency and venture studio focussing on sustainability tech, based in London, UK.
Find out more at [hard-stuff.com](hard-stuff.com).

This library is written and provided open-source in the hope that you go on to build great things.
