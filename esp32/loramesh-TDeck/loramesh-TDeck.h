// loramesh_TDeck.h

// unfortunately the following is not honored, patched the cpp file
#define LV_USE_PERF_MONITOR 0 

#include <Arduino.h>

#include "config.h"  // all definitions and header files for TDeck



// the following files contain actual code (not only API definitions):
#include "ed25519.h"
#if defined(HAS_BT)
#  include "kiss.h"
#endif
#include "node.h"
/*
#include "mgmt.h"
#include "heatmap.h"
*/

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
struct bipf_s *the_config;
struct lora_config_s *the_lora_config;

#include "cmd.h"

// ---------------------------------------------------------------------------

#include "hw_setup.h"

UIClass    *theUI;
DmxClass   *theDmx;
Repo2Class *theRepo;
GOsetClass *theGOset;
PeersClass *thePeers;
SchedClass *theSched;

App_TVA_Class *the_TVA_app;

extern int lora_send_ok;

void theGOset_rx(unsigned char *pkt, int len, unsigned char *aux,
                 struct face_s *f)
{
  theGOset->rx(pkt, len, aux, f);
}

// ---------------------------------------------------------------------------

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  int  cnt = 0;
  while (file) {
    cnt++;
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\r\n", file.name());
      if (levels)
        listDir(fs, file.path(), levels -1);
    } else
      Serial.printf("  FILE: %s\tSIZE: %d\r\n", file.name(), file.size());
    file = root.openNextFile();
  }
  if (cnt == 0)
    Serial.printf("  EMPTY\r\n");
}

// --------------------------------------------------------------------------

void probe_for_goset_vect(unsigned char **pkt,
                          unsigned short *len,
                          unsigned short *reprobe_in_millis)
{
  theGOset->probe_for_goset_vect(pkt, len, reprobe_in_millis);
}

void probe_for_peers_beacon(unsigned char **pkt,
                            unsigned short *len,
                            unsigned short *reprobe_in_millis)
{
  thePeers->probe_for_peers_beacon(pkt, len, reprobe_in_millis);
}


void setup()
{
  char msg[100];

  Serial.begin(115200);
  
  Serial.printf("Welcome to tinySSB (for the Lilygo T-Deck device)\r\n");
  Serial.printf("compiled %s %s%s\r\n\r\n", __DATE__ , __TIME__, UTC_OFFSET);
  delay(1500);
    
  hw_setup();

  theUI = new UIClass();
  theUI->spinner(true);

  theDmx   = new DmxClass();
  theRepo  = new Repo2Class();
  theGOset = new GOsetClass();
  thePeers = new PeersClass();
  theSched = new SchedClass(probe_for_goset_vect,
                            probe_for_peers_beacon,
                            probe_for_want_vect,
                            probe_for_chnk_vect);
  {
    unsigned char h[32];
    crypto_hash_sha256(h, (unsigned char*) GOSET_DMX_STR, strlen(GOSET_DMX_STR));
    memcpy(theDmx->goset_dmx, h, DMX_LEN);
    theDmx->arm_dmx(theDmx->goset_dmx, theGOset_rx, NULL);
    Serial.printf("   DMX for GOST is %s\r\n", to_hex(theDmx->goset_dmx, 7, 0));

    theRepo->load();

    // listDir(MyFS, "/", 2); // FEED_DIR, 2);
    // the_TVA_app = new App_TVA_Class(posts);
    // the_TVA_app->restream();
  }
  theUI->spinner(false);

  Serial.println("end of setup\n");
}

void loop()
{
  unsigned char pkt_buf[200];
  int pkt_len;
  
  // delay(5);

  button.check();
  
  if (Serial.available())
    cmd_rx(Serial.readString());

  loopBLE();
  lora_poll();
  pkt_len = lora_get_pkt(pkt_buf);
  if (pkt_len > 0) {
    // lora_cnt++;
    // theStatus->advance_lora_wheel();
    // Serial.printf("Rcv LoRa %dB\r\n", pkt_len);
    incoming(&lora_face, pkt_buf, pkt_len, 1);
    // sprintf(lora_line, "LoRa %d/%d: %dB, rssi=%d", lora_cnt, lora_bad_crc, pkt_len, LoRa.packetRssi());
  }
  
  theSched->tick();

  // loopRadio();
  // io_dequeue();

  for (int i = 0; i < 5; i++) {
    lv_task_handler();
    delay(1);
  }
}

// eof
