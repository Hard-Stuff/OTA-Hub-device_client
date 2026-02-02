# Custom Providers
**e.g. self-hosted or non Git sources.**

- Providers are how we tell our firmware where to look for updates and how to download them. For GitHub and GitLab this is easy because we always check the latest release, offer up whether to install it, and if so the url is easily built from our repo names/ids.
- You can also build your own provider - this'll either be if:
  - You want more finer-grain control of your Git releases (e.g. regexxing the release files to pre-filter releases)
  - or if you're using your own build repo (e.g. Amazon S3).
- See [github.hpp](./src/OTA-Hub/FOTA-providers/github.hpp) and [gitlab.hpp](./src/OTA-Hub/FOTA-providers/gitlab.hpp) files for examples, and build your new class on top of [generic-provider.hpp](./src/OTA-Hub/FOTA-providers/generic-provider.hpp).
- You will need to source your own `CA Certs` for your own server.

## Building a super simple provider for a super simple checker

_docs coming soon_