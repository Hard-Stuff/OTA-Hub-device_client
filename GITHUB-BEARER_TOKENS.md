# Creating personal access tokens for private GitHub repos

- You need a personal access token as a user, unfortunately repo-specific SSH deploy keys won't work here.
- Follow the below steps to generate a new key.

## 1. Go to your GitHub Settings

<img src="/readme_assets/1 - GitHub Settings.png" width="400px"/>

## 2. Go to developer settings

<img src="/readme_assets/2 - Developer Settings.png" width="400px"/>

## 3. Add a "Fine-grained tokens" (which is better for tighter security)

<img src="/readme_assets/3 - Fine-grained tokens.png" width="400px"/>

## 4. Define your token definitions:

- Give it a memorable and obvious name
- Set the expiration date for the token. Fine grain tokens only last a maximum of 1 year - we are exploring how we can work around this (or move to OTA Hub pro!)
- Fine-grain the token to work on **Only select repositories** - this means it'll not work on any other repos, hence tighter security.

<img src="/readme_assets/4 - Token definitions.png" width="400px"/>

## 5. ..and ensure that only "Contents -> Read-only" is set

- We only need the contents of the repo, i.e. the release contents
- We only need "Read-only" access, as the ESP32 will never be writing on the repo

<img src="/readme_assets/5 - Contents.png" width="400px"/>

## 6. Paste the created token in your OTAGH_BEARER

- Note that even this is admittedly quite insecure, and we are working on ways to improve this security.
- Even more security options are available in OTA Pro.

<img src="/readme_assets/6 - Paste the token in your code.png" width="400px"/>
