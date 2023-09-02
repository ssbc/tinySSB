// device.h

// tinySSB for ESP32, Lilygo T-Deck/T-Wrist version
// (c) 2023 <christian.tschudin@unibas.ch>

// collect all external libraries here


// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TBOARD)
#define DEVICE_MAKE "Lilygo T-Beam"
#endif // TINYSSB_BOARD_TBOARD

// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TDECK)
#define DEVICE_MAKE "Lilygo T-Deck"

#include <SPI.h>

#include <TFT_eSPI.h>
// #include "TFT_eSPI.h"
// #include "es7210.h"
// #include <Audio.h>

#include <driver/i2s.h>

#define TOUCH_MODULES_GT911
#include "TouchLib.h"
#include "AceButton.h"

#include <lvgl.h>

#endif // TINYSSB_BOARD_TDECK

// ---------------------------------------------------------------------------

#if defined(TINYSSB_BOARD_TWRIST)
#define DEVICE_MAKE "Lilygo T-Wrist"

#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
// #include <GxGDEH0154Z90/GxGDEH0154Z90.h>  // 1.54" b/w/r 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#define PIN_MOTOR 4
#define PIN_KEY GPIO_NUM_35
#define PWR_EN 5
#define Backlight 33

#define SPI_SCK 14
#define SPI_DIN 13
#define EPD_CS 15
#define EPD_DC 2
#define SRAM_CS -1
#define EPD_RESET 17
#define EPD_BUSY 16

#endif // TINYSSB_BOARD_TWRIST


// FS -----------------------------------------------------------------

// #include <SD.h>
#include <LittleFS.h>
#define MyFS LittleFS


// BT, LoRa and WiFi --------------------------------------------------
#include <lwip/def.h> // htonl

#if defined(HAS_BLE)
# include <BLEDevice.h>
# include <BLEServer.h>
# include <BLEUtils.h>
# include <BLE2902.h>
# include "esp_gatt_common_api.h"
#endif

#if defined(HAS_BT)
#  include <BluetoothSerial.h>
#endif

#if defined(HAS_LORA)
#if defined(USE_RADIO_LIB)
#  define USING_SX1262
#  include "RadioLib.h"
#else
#  include <LoRa.h>
#endif // USE_RADIO_LIB
#endif

#if defined(HAS_UDP)
#  include <WiFi.h>
#  include <WiFiAP.h>
#endif

// crypto --------------------------------------------------------------------

#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign_ed25519.h>

// eof
