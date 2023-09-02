// config.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collects main configuration details in this header file

#if !defined(_INCLUDE_CONFIG_H)
#define _INCLUDE_CONFIG_H

// ---------------------------------------------------------------------------
// hardware profile:

#if defined(TINYSSB_BOARD_TBOARD)
# define HAS_BLE
# define HAS_LORA
#endif // TINYSSB_BOARD_TBOARD

#if defined(TINYSSB_BOARD_TDECK)
# define HAS_BLE
# define HAS_LORA
# define USE_RADIO_LIB
#endif // TINYSSB_BOARD_TDECK

#if defined(TINYSSB_BOARD_TWRIST)
# define HAS_BLE
#endif // TINYSSB_BOARD_TWRIST

// #define HAS_BLE         // enable Bluetooth Low Energy
// #define HAS_BT          // the ESP32-3C has no Bluetooth
// #define HAS_LORA        // enable LoRa
// #define USE_RADIO_LIB

// #define HAS_UDP
// #define HAS_ ...

// ---------------------------------------------------------------------------

#if !defined(UTC_OFFSET)
# define UTC_OFFSET ""
#endif


// ---------------------------------------------------------------------------

#include <Arduino.h>

#include "device.h"
#include "const.h"

// files which have a corresponding *.cpp:
#include "util.h"
#include "config-tSSB.h"
#include "ui.h"
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

// ---------------------------------------------------------------------------
// global variables and function forward declarations

extern unsigned char my_mac[6];
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
extern char ssid[];
#endif

extern void incoming_pkt(unsigned char* buf, int len, unsigned char *fid, struct face_s *);
extern void incoming_chunk(unsigned char* buf, int len, int blbt_ndx, struct face_s *);
extern void incoming_want_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);
extern void incoming_chnk_request(unsigned char* buf, int len, unsigned char* aux, struct face_s *);

// ---------------------------------------------------------------------------

#endif // _INCLUDE_CONFIG_H
