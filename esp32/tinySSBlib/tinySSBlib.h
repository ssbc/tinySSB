// tinySSBlib.h

// tinySSB for ESP32, Lilygo T-Deck version
// (c) 2023 <christian.tschudin@unibas.ch>

// collects main configuration details in this header file

#if !defined(_INCLUDE_TINYSSBLIB_H)
#define _INCLUDE_TINYSSBLIB_H

// ---------------------------------------------------------------------------

#include <Arduino.h>

#include <string.h>
#include <lwip/def.h> // htonl

#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign_ed25519.h>

#include "device.h"
#include "const.h"

// files which have a corresponding *.cpp:
#include "util.h"
#include "config-tSSB.h"
#include "ui.h"
extern UIClass *theUI;
#include "io.h"
#include "bipf.h"
#include "goset.h"
extern GOsetClass *theGOset;
#include "dmx.h"
extern DmxClass   *theDmx;
#include "replica.h"
#include "repo.h"
extern Repo2Class *theRepo;
#include "sched.h"
extern SchedClass *theSched;
#include "peers.h"
extern PeersClass *thePeers;

// #include "app_tva.h"


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

#endif // _INCLUDE_TINYSSBLIB_H
