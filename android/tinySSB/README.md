# The tinySSB app for Android

The ```tinySSB``` app is a decent(ralized) user interface to the
replicated append-only logs on which this project is build. It has the
following highlights:

- supports several internal "apps":
  - chat, featuring voice messaging
  - Kanban boards for collaborative planning
- has easy onboarding: user IDs from neighboring devices are automatically imported
- works "offline-first" i.e., no Internet connection needed to edit your Kanban board
- offers local Bluetooth Low Energy (BLE) as well as websocket (over the Internet) communication interfaces
- is based on the tinySSB log format (all packets are 120B long) which potentially can also be shipped over narrowband wireless media (LoRa).

Two devices in the same room will immediately start to sync the
content of the tinySSB app using BLE. This requires that Bluetooth is
enabled as well as localization (in order to access BLE), and that the
app is active (no background syncing).

## Notes Regarding Compilation

Compiling the ```tinySSB``` app is challenging because of the included
Codec2 library which must be cross-compiled to native code (for
Android smartphones).

The compilation is currently configured to generate an APK that
runs on devices in the range of Android 8/Oreo to Android 12 (SDK 26 to 31).
If an APK should be compiled that can be published via Google Play,
the minSdkVersion will have to be changed to 30 (instead of 26),
cutting off at the same time owners of phones with Android 10 or below.

## Note Regarding Security

This is a preliminary version of the app, built mostly for demo
purposes. As such it lacks some desirable security properties and
supports only a single, unencrypted public chat channel.

A next version of this app is in preparation: it will be based on
"encryption circles" that provide basic confidentiality and end-to-end
encryption and will also have private person-to-person chats as well
as encrypted Kanban boards.


## Credits

"Codec 2 is an open source speech codec designed for communications
quality speech between 700 and 3200 bit/s."
[Codec2 homepage](http://rowetel.com/codec2.html)

The Android code for the voice recorder and player view is based on this
[demo AudioRecorder app](https://github.com/exRivalis/AudioRecorder)

---
