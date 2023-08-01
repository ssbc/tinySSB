# tinySSB code for ESP32 T-Beam 1.1 and Heltec Lora V1.2 devices

This is the LoRa mesh network code for tinySSB.

How it works: A collection of T-Beam and Heltec devices form a
wireless 'virtual pub' where all new tinySSB content injected at one
node will be available at all other nodes of the mesh. External
devices interface with the mesh via Bluetooth Low Energy.  Typically
you need the tinySSB Android app in order to access the mesh.

- nodes talk to each other via LoRa (1 to 10 miles)
- support for different LoRa profiles (AU915, EU868 and US915)
- nodes talk to non-mesh nodes via Bluetooth Low Energy (30 to 300 feet)
- local management commands via serial-over-USB
- remote management functionality
- logging of LoRa reception
- WiFi and KISS-over-USB are local access options but are disabled by default


---