// config.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// collects main configuration details in this header file

#if !defined(_INCLUDE_CONFIG_H)
#define _INCLUDE_CONFIG_H


#define HAS_OLED
#define HAS_BLE   // enable Bluetooth Low Energy
#define HAS_LORA  // enable LoRa

// FIXME: change the following to positive statements, as above
#define NO_BT    // disable Bluetooth
// #define NO_GPS   // disable GPS
#define NO_WIFI  // disable WiFi

#define SLOW_CPU
#define SLOW_CPU_MHZ 80  // 40MHz is too low for handling the BLE protocol

#define LOG_FLUSH_INTERVAL         10000 // millis
#define LOG_BATTERY_INTERVAL  15*60*1000 // millis (15 minutes)

#define PEERS_FILENAME "/peers-gps.log"
#define PEERS_INTERVAL 20000 // how often to sent pings, in msec

// ----------------------------------------------------------------------

#define BAUD_RATE    115200 // or 38400 or 460800

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
#define tSSB_WIFI_PW     "dWeb2022"
#define tSSB_UDP_ADDR    "239.5.5.8"
#define tSSB_UDP_PORT    1558

#define PKTTYPE_plain48  '\x00' // single packet with 48B payload
#define PKTTYPE_chain20  '\x01' // start of hash sidechain (pkt contains BIPF-encoded content length)

#define BLE_SERVICE_UUID           "6e400001-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_RX "6e400002-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_TX "6e400003-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_ST "6e400004-7646-4b5b-9a50-71becce51558"

#define BLE_RING_BUF_SIZE 3


extern unsigned int bipf_varint_decode(unsigned char *buf, int pos, int *lptr);

extern void io_enqueue(unsigned char *pkt, int len, unsigned char *dmx /*=NULL*/, struct face_s *f /*=NULL*/);
extern void io_send(unsigned char *buf, short len, struct face_s *f /*=NULL*/);

extern void incoming_pkt(unsigned char* buf, int len, unsigned char *fid, struct face_s *);
extern void incoming_chunk(unsigned char* buf, int len, int blbt_ndx, struct face_s *);
extern void incoming_want_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);
extern void incoming_chnk_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);
extern void ble_init();

extern void hm_init();
extern void hm_tick();

extern struct bipf_s *the_config;
extern struct lora_config_s *the_lora_config;

#include <Arduino.h>

#define MyFS LittleFS

#include "device.h"
#include "status.h"
#include "util.h"
#include "io.h"
#include "bipf.h"
#include "goset.h"
extern GOsetClass *theGOset;
#include "dmx.h"
extern DmxClass   *dmx;
#include "replica.h"
#include "repo.h"
extern Repo2Class *repo;
#include "status.h"
extern StatusClass *theStatus;

// ---------------------------------------------------------------------------

struct lora_config_s {
  char plan[8];        // name of frequency plan, ASCIIZ
  unsigned long  fr;   // in Hz
  unsigned int   bw;   // bandwidth, in Hz
  unsigned short sf;   // spreading factor (7-12)
  unsigned short cr;   // coding rate (5-8)
  unsigned char  sw;   // sync word
  unsigned char  tx;   // tx power
};

extern struct lora_config_s lora_configs[];
extern short lora_configs_size;

extern int lora_prssi;
extern float lora_psnr;

// ---------------------------------------------------------------------------

// the "config.bipf: file retains persisted config choices as a dict
// currently used:

// {
//   "lora_plan": "US915", // i.e., the lora config is specified by plan name
// }

#define CONFIG_FILENAME "/config.bipf"

struct bipf_s* config_load(); // returns a BIPF dict with the persisted config dict
void           config_save(struct bipf_s *dict); // persist the BIPF dict

#endif // _INCLUDE_CONFIG_H
