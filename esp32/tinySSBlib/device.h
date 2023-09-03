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

#if defined(TINYSSB_BOARD_TBOARD)
# define DEVICE_MAKE "Lilygo T-Beam"
# define HAS_BLE
# define HAS_LORA
#endif // TINYSSB_BOARD_TBOARD

// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TDECK)
# define DEVICE_MAKE "Lilygo T-Deck"
# define HAS_BLE
# define HAS_LORA
# define USE_RADIO_LIB

# define USING_SX1262
# include "RadioLib.h"
  extern SX1262 radio;
# define RADIO_CS_PIN        9
# define RADIO_BUSY_PIN      13
# define RADIO_RST_PIN       17
# define RADIO_DIO1_PIN      45
#endif // TINYSSB_BOARD_TDECK

// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TWRIST)
# define DEVICE_MAKE "Lilygo T-Wrist"
# define HAS_BLE
#endif // TINYSSB_BOARD_TWRIST


// FS -----------------------------------------------------------------

// #include <SD.h>
#include <LittleFS.h>
#define MyFS LittleFS

// eof
