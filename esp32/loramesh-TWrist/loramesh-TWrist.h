// loramesh-TWrist.h

#include "src/config.h"
#include "src/node.h"

#include <Fonts/FreeMonoBold9pt7b.h>

GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);

struct bipf_s *the_config;
unsigned char my_mac[6];
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
char ssid[sizeof(tSSB_WIFI_SSID) + 6];
#endif

DmxClass   *theDmx;
UIClass    *theUI;
Repo2Class *theRepo;
GOsetClass *theGOset;
PeersClass *thePeers;
SchedClass *theSched;

void theGOset_rx(unsigned char *pkt, int len, unsigned char *aux,
                 struct face_s *f)
{
  theGOset->rx(pkt, len, aux, f);
}

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

// ---------------------------------------------------------------------------

/*
void DeepSleep(void)
{
  Serial.println("switching to deep sleep mode");

  gXdisplay.powerDown();
  pinMode(PIN_MOTOR, INPUT_PULLDOWN);
  pinMode(PWR_EN, INPUT);
  pinMode(PIN_KEY, INPUT);
  pinMode(SPI_SCK, INPUT);
  pinMode(SPI_DIN, INPUT);
  pinMode(EPD_CS, INPUT);
  pinMode(EPD_DC, INPUT);
  pinMode(EPD_RESET, INPUT);
  pinMode(EPD_BUSY, INPUT);
  pinMode(PIN_MOTOR, INPUT);
  pinMode(Backlight, INPUT),
  esp_sleep_enable_ext0_wakeup(PIN_KEY, 0);
  esp_deep_sleep_start();
}
*/

void setup()
{
  Serial.begin(115200);
  delay(10);

  Serial.println("\r\ntinySSB virtual pub for the Lilygo T-Wrist device");

  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PWR_EN, OUTPUT);

  gXdisplay.init();
  gXdisplay.setRotation(0);
  // display.invertDisplay(true);
  gXdisplay.fillScreen(GxEPD_BLACK);
  gXdisplay.drawBitmap(0, 0, scuttleshell, 200, 200, GxEPD_WHITE);
  gXdisplay.update();

  delay(1000);
  gXdisplay.setFont(&FreeMonoBold9pt7b);
  gXdisplay.setTextColor(GxEPD_WHITE);
  gXdisplay.setCursor(60, 40);
  gXdisplay.println("hello1");
  gXdisplay.setTextColor(GxEPD_BLACK);
  gXdisplay.setCursor(60, 60);
  gXdisplay.println("hello2");
  gXdisplay.update();

  // DeepSleep(); --------------------------------------------------

  if (!MyFS.begin(true)) {
    Serial.println("LittleFS Mount Failed, partition was reformatted");
    // return;
  }
  // MyFS.format(); // uncomment and run once after a change in partition size
  MyFS.mkdir(FEED_DIR);
  Serial.printf("LittleFS: %d total bytes, %d used\r\n",
                MyFS.totalBytes(), MyFS.usedBytes());

  the_config = config_load();
  /*
  struct bipf_s k = { BIPF_STRING, 9, {.str = "lora_plan"} };
  struct bipf_s *v = bipf_dict_getref(the_config, &k);

  if (v != NULL && v->typ == BIPF_STRING &&
                                    strncmp("US902.5", v->u.str, v->cnt)) {
    bipf_dict_set(the_config, bipf_mkString("lora_plan"),
                  bipf_mkString("US902.5"));
    config_save(the_config);
  }
  */

  // FIXME: we should not print the mgmt signing key to the console ?
  Serial.printf("tinySSB config is %s\r\n",
                bipf2String(the_config, "\r\n", 1).c_str());
  Serial.println();


  esp_efuse_mac_get_default(my_mac);
#if !defined(HAS_UDP)  // in case of no Wifi: display BT mac addr, instead
  my_mac[5] += 2;
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.0/api-reference/system/base_mac_address.html
#endif
  
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2));
  Serial.printf("This is node %s\r\n\r\n", ssid);
#endif
  io_init();

  theDmx   = new DmxClass();
  theUI    = new UIClass();
  theUI->to_next_screen();
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

    Serial.printf("\r\n   %d feeds, %d entries, %d chunks\r\n",
                  theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt);
    // listDir(MyFS, "/", 2); // FEED_DIR, 2);
    // the_TVA_app = new App_TVA_Class(posts);
    // the_TVA_app->restream();
  }

  Serial.println("end of setup\n");

  digitalWrite(PWR_EN, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
  delay(100);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
}


void loopBLE()
{
#if defined(MAIN_BLEDevice_H_) && !defined(NO_BLE)
  unsigned char *cp = ble_fetch_received();
  if (cp != NULL)
    incoming(&ble_face, cp+1, *cp, 0);
#endif
}


void loop()
{
  theSched->tick();

  loopBLE();
  
  delay(5);
}
