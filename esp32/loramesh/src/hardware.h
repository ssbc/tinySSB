// hardware.h

#ifndef _INCLUDE_HARDWARE_H
#define _INCLUDE_HARDWARE_H

extern void hw_init();

// collects hardware-specific constants for all boards

// ---------------------------------------------------------------------------
#if defined(TINYSSB_BOARD_HELTEC) || defined(TINYSSB_BOARD_HELTEC3)

// # define SCK     5    // GPIO5  -- SX1278's SCK
// # define MISO    19   // GPIO19 -- SX1278's MISO
// # define MOSI    27   // GPIO27 -- SX1278's MOSI
// # define SS      18   // GPIO18 -- SX1278's CS
# define RST     14   // GPIO14 -- SX1278's RESET
# define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
# define DI1     35
# define DI2     34

// SPI.begin(5, 19, 27, 18);
// LoRa.setPins(SS, RST, DI0);

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_T5GRAY

// epaper device is GDEW0213T5

#define SPI_SCK    18 // 14 // CLK
#define SPI_DIN    23 // 13

#define SRAM_CS -1

#define EPD_CS      5 // 15
#define EPD_DC     17 // 2
#define EPD_RESET  16 // 17
#define EPD_BUSY    4 // 16

#define PIN_KEY GPIO_NUM_39

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TBEAM

#if defined(HAS_LORA) && !defined(USE_RADIO_LIB)
# include <SPI.h>
# include <LoRa.h>
#endif

#ifdef TBEAM_07
# define GPS_TX 12
# define GPS_RX 15
#endif

// # define SCK     5    // GPIO5  -- SX1278's SCK
// # define MISO    19   // GPIO19 -- SX1278's MISO
// # define MOSI    27   // GPIO27 -- SX1278's MOSI
// # define SS      18   // GPIO18 -- SX1278's CS
# define RST     14   // GPIO14 -- SX1278's RESET
# define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

// cs/busy/rst/di01

// tbeam 2: 10, 4, 5, 1    according to meshtastic
// tbeam 2: 5, 33, 13, 14  according to https://meshtastic.discourse.group/t/tbeam-with-sx1262/972/24
// tbeam 1: 18, 32, 23, 33  according to meshtastic
// tbeam 1: 18, 26?, 23, ??  according to 
#define RADIO_CS_PIN        18
#define RADIO_BUSY_PIN      x
#define RADIO_RST_PIN       14
#define RADIO_DIO1_PIN      26

// user button
#ifdef TBEAM_07
# define BUTTON_PIN 39
#else
# define BUTTON_PIN 38  // this is for T_BEAM_V10
#endif

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TDECK

/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-04-11
 *
 */

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

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TWRIST

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

#endif

// ---------------------------------------------------------------------------

#endif // _INCLUDE_HARDWARE_H
// eof
