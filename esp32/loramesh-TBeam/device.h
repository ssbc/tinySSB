// device.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// collect all external libraries here

// ----------------------------------------------------------------------
#if defined(ARDUINO_heltec_wifi_lora_32_V2)

# include <heltec.h>

# define NO_GPS
# define theDisplay (*Heltec.display)

// user button
#define BUTTON_PIN KEY_BUILTIN  // for heltec?

// ----------------------------------------------------------------------
#elif defined(ARDUINO_TBEAM_USE_RADIO_SX1262) // ARDUINO_TBeam

# include <axp20x.h>
# include <Wire.h>

#if defined(HAS_LORA)
# include <LoRa.h>
#endif

#if !defined(NO_OLED)
# include <SSD1306.h> // display
  extern SSD1306 theDisplay; // lilygo t-beam
#endif

// GPS
#if !defined(NO_GPS)
# include <TinyGPS++.h>
#endif

# define SCK     5    // GPIO5  -- SX1278's SCK
# define MISO    19   // GPIO19 -- SX1278's MISO
# define MOSI    27   // GPIO27 -- SX1278's MOSI
# define SS      18   // GPIO18 -- SX1278's CS
# define RST     14   // GPIO14 -- SX1278's RESET
# define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

// user button
#define BUTTON_PIN 38 // this is for T_BEAM_V10; V7 used pin 39

#endif // device specific
// ----------------------------------------------------------------------


// device-unspecific includes:

// user button
#include <Button2.h>

// file system
#include <LittleFS.h>

// crypto
// #include <sodium/crypto_hash_sha256.h>
// #include <sodium/crypto_sign_ed25519.h>

// WiFi and BT
#if !defined(NO_WIFI)
#  include <WiFi.h>
#  include <WiFiAP.h>
#endif
#include <lwip/def.h> // htonl
#if !defined(NO_BT)
#  include <BluetoothSerial.h>
#endif

// BLE
#if defined(HAS_BLE)
# include <BLEDevice.h>
# include <BLEServer.h>
# include <BLEUtils.h>
# include <BLE2902.h>
# include "esp_gatt_common_api.h"
#endif


// eof
