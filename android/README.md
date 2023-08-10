# Android Apps for tinySSB

This directory collect the source code for tinySSB apps running on
Android phones. Most prominent is ```tinySSB``` which is the user
interface to the replicated tinySSB logs.

- [tinySSB](tinySSB) -- features chat with voice messaging (Codec2), Kanban boards (collaborative planning), log synchronization via Bluetooth Low Energy (BLE) as well as WebSocket. Thanks to its BLE interface, two devices in the same room can immediately synchronize their tinySSB content, import user IDs etc.
- more to come here

## Notes

Compiling the ```tinySSB``` app is challenging because of the included
Codec2 library which must be cross-compiled to native code (for
Android smartphones). If on Windows you may have to change the file
```tinySSB/app/src/main/codec2/src/CMakeLists.txt``` and add a
```.exe``` suffix to a file name, see
```tinySSB/app/src/main/codec2/src/CMakeLists-for-windows.txt```.

---
