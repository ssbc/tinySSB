// config.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collects main configuration details in this header file

#if !defined(_INCLUDE_CONFIG_H)
#define _INCLUDE_CONFIG_H


// #define HAS_BLE   // enable Bluetooth Low Energy
#define HAS_LORA  // enable LoRa
// HAS_ ...

// ---------------------------------------------------------------------------

#include <Arduino.h>

#include "device.h"

#endif // _INCLUDE_CONFIG_H
