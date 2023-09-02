// device.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collect all external libraries here

// #include <SPI.h>
/*
#include <TFT_eSPI.h>
// #include "TFT_eSPI.h"
// #include "es7210.h"
// #include <Audio.h>
*/
// #include <driver/i2s.h>

// #define TOUCH_MODULES_GT911
// #include "TouchLib.h"
// #include "AceButton.h"

// #include <lvgl.h>

// file system --------------------------------------------------------
// #include <SD.h>
#include <LittleFS.h>


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
