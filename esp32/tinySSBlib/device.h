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

#if defined(TINYSSB_BOARD_TBEAM)
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

#if defined(TINYSSB_BOARD_TDECK)
# define DEVICE_MAKE "Lilygo T-Deck"
# define HAS_BLE
# define HAS_LORA

# define USE_RADIO_LIB
# define USING_SX1262
#endif // TINYSSB_BOARD_TDECK

// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TWRIST)
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

// eof
