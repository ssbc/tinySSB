// config.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collects main configuration details in this header file

#if !defined(_INCLUDE_CONFIG_H)
#define _INCLUDE_CONFIG_H

#define HAS_BLE         // enable Bluetooth Low Energy
// #define HAS_BT          // the ESP32-3C has no Bluetooth
// #define HAS_LORA        // enable LoRa
// #define USE_RADIO_LIB

// #define HAS_UDP
// #define HAS_ ...


#define PEERS_INTERVAL 20000 // how often to sent pings, in msec

// ---------------------------------------------------------------------------

#if !defined(UTC_OFFSET)
# define UTC_OFFSET ""
#endif

#define LORA_LOG // enable macro for logging received pkts
#define LORA_LOG_FILENAME  "/lora_log.txt"

#define FID_LEN         32
#define HASH_LEN        20
#define FID_HEX_LEN     (2*FID_LEN)
#define FID_B64_LEN     ((FID_LEN + 2) / 3 * 4)
#define FEED_DIR        "/feeds"
#define MAX_FEEDS       100
#define FEED_PATH_SIZE  sizeof(FEED_DIR) + 1 + 2 * FID_LEN

#define DMX_LEN          7
#define GOSET_DMX_STR    "tinySSB-0.1 GOset 1"

#define TINYSSB_PKT_LEN  120
#define TINYSSB_SCC_LEN  (TINYSSB_PKT_LEN - HASH_LEN) // sidechain content per packet

#define tSSB_WIFI_SSID   "tinySSB"
#define tSSB_WIFI_PW     "password"
#define tSSB_UDP_ADDR    "239.5.5.8"
#define tSSB_UDP_PORT    1558

#define PKTTYPE_plain48  '\x00' // single packet with 48B payload
#define PKTTYPE_chain20  '\x01' // start of hash sidechain (pkt contains BIPF-encoded content length)

#define BLE_SERVICE_UUID           "6e400001-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_RX "6e400002-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_TX "6e400003-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_ST "6e400004-7646-4b5b-9a50-71becce51558"

#define BLE_RING_BUF_SIZE 3

#define NODE_ROUND_LEN   14000  // millis, for GOset, 1/2 is basis for WANT/CHNK

// ---------------------------------------------------------------------------

#include <Arduino.h>
// #include <Stream.h>

#define MyFS LittleFS

#include "device.h"

#if defined(TINYSSB_BOARD_TWRIST)

#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
// #include <GxGDEH0154Z90/GxGDEH0154Z90.h>  // 1.54" b/w/r 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include "scuttleshell.h"

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

extern unsigned char my_mac[6];

// those files which have a corresponding *.cpp:
#include "util.h"
#include "config-tSSB.h"
// #include "ui.h"
#include "io.h"
#include "bipf.h"
#include "goset.h"
extern GOsetClass *theGOset;
#include "dmx.h"
extern DmxClass   *theDmx;
#include "replica.h"
#include "repo.h"
extern Repo2Class *theRepo;
// #include "app_tva.h"
#include "sched.h"
extern SchedClass *theSched;
#include "peers.h"
extern PeersClass *thePeers;


#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
void newLoraPacket_cb(void);
#endif

#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  extern SX1262 radio;
#endif


#if defined(HAS_BT)
extern BluetoothSerial BT;
extern void kiss_write(Stream &s, unsigned char *buf, short len);
#endif

// extern void io_enqueue(unsigned char *pkt, int len, unsigned char *dmx /*=NULL*/, struct face_s *f /*=NULL*/);
// extern void io_send(unsigned char *buf, short len, struct face_s *f /*=NULL*/);

extern void incoming_pkt(unsigned char* buf, int len, unsigned char *fid, struct face_s *);
extern void incoming_chunk(unsigned char* buf, int len, int blbt_ndx, struct face_s *);
extern void incoming_want_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);
extern void incoming_chnk_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);

extern void ble_init();

// ---------------------------------------------------------------------------

#endif // _INCLUDE_CONFIG_H
