// device.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collect all external libraries here

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

// file system --------------------------------------------------------
#include <SD.h>
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


// ---------------------------------------------------------------------------

/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-04-11
 *
 */
#pragma once



//! The board peripheral power control pin needs to be set to HIGH when using the peripheral
#define BOARD_POWERON       10

#define BOARD_I2S_WS        5
#define BOARD_I2S_BCK       7
#define BOARD_I2S_DOUT      6

#define BOARD_I2C_SDA       18
#define BOARD_I2C_SCL       8

#define BOARD_BAT_ADC       4

#define BOARD_TOUCH_INT     16
#define BOARD_KEYBOARD_INT  46

#define BOARD_SDCARD_CS     39
#define BOARD_TFT_CS        12
#define RADIO_CS_PIN        9

#define BOARD_TFT_DC        11
#define BOARD_TFT_BACKLIGHT 42

#define BOARD_SPI_MOSI      41
#define BOARD_SPI_MISO      38
#define BOARD_SPI_SCK       40

#define BOARD_TBOX_G02      2
#define BOARD_TBOX_G01      3
#define BOARD_TBOX_G04      1
#define BOARD_TBOX_G03      15

#define BOARD_ES7210_MCLK   48
#define BOARD_ES7210_LRCK   21
#define BOARD_ES7210_SCK    47
#define BOARD_ES7210_DIN    14

#define RADIO_BUSY_PIN      13
#define RADIO_RST_PIN       17
#define RADIO_DIO1_PIN      45

#define BOARD_BOOT_PIN      0

#define BOARD_BL_PIN        42


/*
// ---------------------------------------------------------------------------
// from meshtastic, Aug 14, 2023
// https://github.com/meshtastic/firmware/blob/master/variants/t-deck/variant.h

// ST7789 TFT LCD
#define ST7789_CS 12
#define ST7789_RS 11  // DC
#define ST7789_SDA 41 // MOSI
#define ST7789_SCK 40
#define ST7789_RESET -1
#define ST7789_MISO 38
#define ST7789_BUSY -1
#define ST7789_BL 42
#define ST7789_SPI_HOST SPI2_HOST
#define ST7789_BACKLIGHT_EN 42
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define TFT_HEIGHT 320
#define TFT_WIDTH 240
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0
#define TFT_OFFSET_ROTATION 0
#define SCREEN_ROTATE
#define SCREEN_TRANSITION_FRAMERATE 5
*/

/*
// LoRa
#define USE_SX1262
#define USE_SX1268

#define RF95_SCK 40
#define RF95_MISO 38
#define RF95_MOSI 41
#define RF95_NSS 9

#define LORA_DIO0 -1 // a No connect on the SX1262 module
#define LORA_RESET 17
#define LORA_DIO1 45 // SX1262 IRQ
#define LORA_DIO2 13 // SX1262 BUSY
#define LORA_DIO3    // Not connected on PCB, but internally on the TTGO SX1262, if DIO3 is high the TXCO is enabled

#define SX126X_CS RF95_NSS // FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_DIO2
#define SX126X_RESET LORA_RESET
#define SX126X_E22 // Not really an E22 but TTGO seems to be trying to clone that
// Internally the TTGO module hooks the SX1262-DIO2 in to control the TX/RX switch (which is the default for the sx1262interface
// code)
*/

// eof
