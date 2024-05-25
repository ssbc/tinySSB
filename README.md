# tinySSB - the LoRa descendant of Secure Scuttlebutt

![tinySSB logo](doc/_img/tinySSB-banner.png)

## Overview

### Tutorial for first build under Windows
#### Prerequisites
- CMake version 3.18.1 or higher
- gcc
- ninja-build

#### Steps
1. Clone the repository
2. **Don't** open the project in Android Studio yet
3. In the folder android/tinySSB, create a new file named "local.properties" and add the following line:
```sdk.dir=C\:\\Users\\<your-username>\\AppData\\Local\\Android\\Sdk```
4. Import the project in Android Studio (File -> Open -> select the file build.gradle in the android/tinySSB folder)

---
