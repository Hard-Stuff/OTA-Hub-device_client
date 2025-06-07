# **OTA Hub (by Hard Stuff)** - OTA directly from GitHub

## MAJOR CHANGE ANNOUNCEMENT

We have merged the DIY library with the pro library to give one smooth library that offers more customisation and easier usage.

## GitHub release usage

For GitHub release usage (i.e. DIY) _please read the [the docs](https://github.com/Hard-Stuff/OTA-Hub-diy-example_project) for more a wider description of **OTA Hub**._

-   Once your GitHub CI/CD is set up on your public or private repos they will automatically create `firmware.bin` files for every newly created release.
-   Next, configure your ESP32 devices to automatically check for the latest releases on your GitHub repository, and download and install the updates as needed.

**OTA Hub** is designed to do one thing, and one thing only:

    Deliver Over-the-Air updates onto your ESP32/embedded devices directly from your code releases in an obvious, clean, light-weight way.

    It is designed to do this on an abstract Client - this means it works equally well over WiFi (and WiFiClientSecure) as it does over 4G/5G clients such as TinyGSM.

**OTA Hub DIY** is for the hobbyists and small teams, directly grabbing release files from GitHub, involving as minimal setup as possible. It's completely open-source, and of course, free!

**OTA Hub Pro** is for the experts and larger teams, providing you with a dashboard to have finer control over your release deployment, fleet management, greater flexibility, and even less setup! Find out more at [ota-hub.com/pro](ota-hub.com/pro).

See our [4G example](./examples/SIM7600/) for a SIM7600 HTTPS implementation of this.

### Benefits over alternatives

1. No-longer worry about drag-and-dropping `.bin` files into some fiddly UI.
2. Easily trace your code to your releases to your deployed firmware.
3. Not locked into any eco-systems you probably aren't using already (you're probably already using GitHub).
4. Client-agnostic! Implement OTA Hub on-top of secure or insecure\* connections, on 4G, NB-IoT, or WiFi modules.
5. The DIY version is open-source, **completely free**, and GitHub hosting is also (currently) completely free!

_\* Note that our default examples are for SSL-enabled connections, as GitHub requires a secure connection. As this is open-source, you can of course use your own storage buckets APIs for insecure connections etc._

## Usage

_You must first have CI/CD set up on your firmware repo of choice. Follow [the docs](https://github.com/Hard-Stuff/OTA-Hub-diy-example_project) for a simple copy-paste guide on how to do this -> you can use the above example project as a robust template._

The flow logic for this entire OTA library is super simple:

1. **Check for updates** - It first checks on your GitHub repo for the latest release of your firmware. GitHub reports back the `name` and `published_at` timestamp of the latest **release**.
2. **Perform the update** - If given the information in step 1 compared to the current installation you want to perform the update: automatically download and install the `firmware.bin` file onto the device.
3. **Follow the redirect** - Because GitHub hosts the release data on `api.github.com` but the `firmware.bin` asset on `objects.githubusercontent.com`, we may have to manually follow the redirect **after** updating the SSL CA Cert. _This suck, but it's how GitHub has built it 🤷‍♂️._

### Basic Example

```cpp
// OTA Hub via GitHub
#define OTAGH_OWNER_NAME "Hard-Stuff"
#define OTAGH_REPO_NAME "OTA-Hub-diy-example_project"
#include <OTA-Hub-diy.hpp>

// Networking
#include <configs/wifi.h>
#include <WiFiClientSecure.h>
WiFiClientSecure wifi_client;

void setup()
{
    // Initialise our board
    Serial.begin(115200);
    Serial.println("Started...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("WiFi failure");
        ESP.restart();
    }

    // Initialise OTA
    // wifi_client.setInsecure(); // if you're worried about expiring CA certs and you don't need to validate SSL
    wifi_client.setCACert(OTAGH_CA_CERT); // Set the api.github.cm SSL cert on the WiFi Client
    OTA::init(wifi_client);

    // 1. Check for updates, by checking the latest release on GitHub
    // .. (n.b. get more control on OTA Hub Pro!)
    OTA::UpdateObject details = OTA::isUpdateAvailable();

    if (OTA::NO_UPDATE == details.condition)
    {
        Serial.println("No new update available. Continuing...");
    }
    else
    // 2. Perform the update (if there is one)
    {
        if (OTA::performUpdate(&details) == OTA::SUCCESS) {
            // .. success! It'll restart by default, or you can do other things here...
        }
        // By default we follow any redirects internally, but should you need to set custom certs you can do that by added a (...), false);

    }
    // As normal... note: performUpdate will restart the board unless you specify otherwise.
}

void loop()
{
    // As normal...
}
```

### `#define`s to be aware of

```cpp
// Required defines
#define OTAGH_OWNER_NAME "Hard-Stuff" // change to the repo's owner
#define OTAGH_REPO_NAME "OTA-Hub-diy-example_project" // chante to the repo's name

// Optional defines
#define OTAGH_BEARER "YOUR PRIVATE REPO TOKEN" // Needed for private repositories, see GITHUB-BEARER_TOKENS.md

// If you are using another server (i.e. not github) you can set
#define OTA_SERVER char*  // default: api.github.com
#define OTA_PORT number   // default: 443 (HTTPS secured)
#define OTA_CHECK_PATH .. // only change if you're not using GitHub
#define OTA_BIN_PATH ..   // only change if you're not using GitHub
#define OTA_BEARER ..     // bearer token for your custom server
                            // you will also need to set custom certs if not using GitHub

// Built-in CA Certs
static const char OTAGH_CA_CERT[];          // CA Cert for GitHub's api.github.com and ...github.io servers We can make NO guarantee that these will remain valid indefinitely!
```

### Dependencies

-   arduino-libraries/ArduinoHttpClient
-   paulstoffregen/Time
-    [hard-stuff/Http](https://registry.platformio.org/libraries/hard-stuff/Http)

### Note on CA Certs

- Typically you will need two CA certs if working as standard with GitHub: one for api.github.com, and one for objects.githubusercontent.com.
- **Certificates expire!** They tend to last a while, these ones last until 2030, but that's something to be aware of. Once either certificate has expired your devices will not be able to perform OTA (until flashed with new certs) - this is something we're going to attempt to future-proof going forwards.
- Plus, as we've experienced recently, certificates that are in date [might just stop working](https://news.ycombinator.com/item?id=35295216). In which case it's not a bad idea to either have a fallback option (we recommend [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) for local flashing) or to watch this space for our future proofing.
- You can always set your own certs (should the default ones not work) via `wifi_client.setCACert(NEW CA CERT)` and `wifi_client.setCACert(NEW CA CERT)`.
- Or you can set `wifi_client.setInsecure()` and remove the `setCACerts...`. This means that the ESP32 will not validate if the api.github.com and objects.githubusercontent.com have the correct CA Certs, so could technically open up security issues (although for hobbyist / non-critical projects this should be fine). 

## Compabibility and testing

This library has been tested on the ESP32S3 with both the internal WiFi functionality and a [SIMCOM SIM7600G](https://github.com/Hard-Stuff/TinyGSM).

We are looking for people to support us in testing more boards, other connectivity functionalities, and making **OTA Hub Pro** even more useful. Contribute either on our GitHub repos, or contact us at [ota-hub@hard-stuff.com](mailto:ota-hub@hard-stuff.com).

## Hard Stuff

Hard Stuff is a hardware prototyping agency and venture studio focussing on sustainability tech, based in London, UK.
Find out more at [hard-stuff.com](hard-stuff.com).

This library is written and provided open-source in the hope that you go on to build great things.
