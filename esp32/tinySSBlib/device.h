// device.h

// tinySSB for ESP32, Lilygo T-Deck/T-Wrist version
// (c) 2023 <christian.tschudin@unibas.ch>

// ---------------------------------------------------------------------------
// hardware profile:

// #define HAS_BLE         // enable Bluetooth Low Energy
// #define HAS_BT          // the ESP32-3C has no Bluetooth
// #define HAS_LORA        // enable LoRa
// #define USE_RADIO_LIB

// #define HAS_UDP
// #define HAS_ ...

// ---------------------------------------------------------------------------

#ifdef TINYSSB_BOARD_HELTEC
// https://heltec.org/project/wifi-lora-32/
# define DEVICE_MAKE "Heltec LoRa32 v2"
# define HAS_BLE
# define HAS_LORA
# define HAS_OLED

// # define USE_RADIO_LIB // try out both (RadioLib and LoRa)
// # define USING_SX1262

# define USE_LORA_LIB

#endif // TINYSSB_BOARD_HELTEC

// ---------------------------------------------------------------------------

#ifdef TINYSSB_BOARD_T5GRAY
// https://www.lilygo.cc/products/t5-2-13inch-e-paper
# define DEVICE_MAKE "Lilygo T5-Grayscale"
# define HAS_BLE

#endif // TINYSSB_BOARD_T5GRAY

// ---------------------------------------------------------------------------

#ifdef TINYSSB_BOARD_TBEAM
// https://www.lilygo.cc/products/t-beam-v1-1-esp32-lora-module
# define DEVICE_MAKE "Lilygo T-Beam"
# define HAS_BLE
# define HAS_GPS
# define HAS_LORA
# define HAS_OLED

// # define USE_RADIO_LIB // try out both (RadioLib and LoRa)
// # define USING_SX1262

# define USE_LORA_LIB

#endif // TINYSSB_BOARD_TBEAM

// ---------------------------------------------------------------------------

#ifdef TINYSSB_BOARD_TDECK
// https://www.lilygo.cc/products/t-deck
# define DEVICE_MAKE "Lilygo T-Deck"
# define HAS_BLE
# define HAS_LORA

# define USE_RADIO_LIB
# define USING_SX1262
#endif // TINYSSB_BOARD_TDECK

// ---------------------------------------------------------------------------

#ifdef TINYSSB_BOARD_TWRIST
// https://www.lilygo.cc/products/t-wrist-e-paper-1-54-inch-display
# define DEVICE_MAKE "Lilygo T-Wrist"
# define HAS_BLE

#endif // TINYSSB_BOARD_TWRIST




// FS -----------------------------------------------------------------

#ifdef USE_RADIO_LIB
# include "RadioLib.h"
  extern SX1262 radio;
#endif

#ifdef USE_LORA_LIB
# include "LoRa.h"
#endif

// #include <SD.h>
#include <LittleFS.h>
#define MyFS LittleFS


// GPS ----------------------------------------------------------------

#ifdef HAS_GPS
# include <TinyGPS++.h>
  extern TinyGPSPlus gps;
  extern HardwareSerial GPS;
#endif


// eof
