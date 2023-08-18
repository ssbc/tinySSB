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

struct bipf_s *the_config;
struct lora_config_s *the_lora_config;

// ---------------------------------------------------------------------------

#include "hw_setup.h"
#include "ui_setup.h"

DmxClass   *theDmx   = new DmxClass();
Repo2Class *theRepo  = new Repo2Class();
GOsetClass *theGOset = new GOsetClass();

App_TVA_Class *thePosts;

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

void setup()
{
  char msg[100];

  Serial.begin(115200);
  
  Serial.printf("Welcome to tinySSB (for the Lilygo T-Deck device)\r\n");
  Serial.printf("compiled %s %s%s\r\n\r\n", __DATE__ , __TIME__, UTC_OFFSET);
  delay(1500);
    
  hw_setup();
  ui_setup();

  unsigned char h[32];
  crypto_hash_sha256(h, (unsigned char*) GOSET_DMX_STR, strlen(GOSET_DMX_STR));
  memcpy(theDmx->goset_dmx, h, DMX_LEN);
  theDmx->arm_dmx(theDmx->goset_dmx, theGOset_rx, NULL);
  Serial.printf("   DMX for GOST is %s\r\n", to_hex(theDmx->goset_dmx, 7, 0));

  theRepo->load();

  // listDir(MyFS, "/", 2); // FEED_DIR, 2);

  thePosts = new App_TVA_Class(posts);
  thePosts->restream();
}

void loop()
{
  delay(5);

  button.check();
  
  theGOset->tick();
  node_tick();

  // loopRadio();
  loopBLE();
  io_dequeue();
  
  lv_task_handler();

  /*
  static long int next_lora;
  char msg[100];

  if (millis() > next_lora) {
    char *s = "hello world";
    io_send((unsigned char*)s, strlen(s));
    io_dequeue();
    next_lora = millis() + 2000;
    sprintf(msg, "%d: sent LoRa %d - %d", millis()%1000000, (next_lora/1000)%16, lora_send_ok);
    Serial.println(msg);
    prep2log(msg);
  }
  */

}

// eof
