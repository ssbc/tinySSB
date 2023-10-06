// loramesh.h

#include "src/lib/tinySSBlib.h"

#include "src/ui-heltec.h"
#include "src/ui-t5gray.h"
#include "src/ui-tbeam.h"
#include "src/ui-tdeck.h"
#include "src/ui-twrist.h"

#include "src/hardware.h"

#if !defined(UTC_OFFSET)
# define UTC_OFFSET ""
#endif

// include files with actual code and data structures:
#include "src/lib/node.h"
#if defined(HAS_BT)
# include "src/lib/kiss.h"
#endif

#include "./src/lib/la_wifi_web.h"

struct bipf_s *the_config;

unsigned char my_mac[6];
char ssid[sizeof(tSSB_WIFI_SSID) + 6];

#ifdef USE_RADIO_LIB
# if defined(TINYSSB_BOARD_TBEAM) || defined(TINYSSB_BOARD_TDECK)
   SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                             RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
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

File lora_log;
File peers_log;

long next_lora_log;
long next_serial_ts;
#define LORA_LOG_INTERVAL  (5*60*1000) // every 5 minutes
#define SERIAL_TS_INTERVAL (15*1000)   // every 15 sec

char* _ts()
{
  static char buf[40];

#ifdef HAS_GPS
  if (gps.date.isValid() && gps.date.year() != 2000) {
    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
            gps.date.year(), gps.date.month(), gps.date.day(),
            gps.time.hour(), gps.time.minute(), gps.time.second());
    return buf;
  }
#endif
  long now = millis();
  sprintf(buf, "%d.%03d", now/1000, now%1000);
  return buf;
}

void _wr(File f, char *fmt, va_list ap)
{
  char buf[250];
  vsprintf(buf, fmt, ap);
  f.printf("%s %s\r\n", _ts(), buf);
}

void lora_log_wr(char *fmt, ...)
{
  va_list args; va_start(args, fmt); _wr(lora_log, fmt, args); va_end(args);
}

void peers_log_wr(char *fmt, ...)
{
  va_list args; va_start(args, fmt); _wr(peers_log, fmt, args); va_end(args);
}

void time_stamp()
{
  if (next_lora_log < millis()) {
    lora_log_wr("F=%d E=%d C=%d |dmxt|=%d |chkt|=%d |heap|=%d",
                theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt,
                theDmx->dmxt_cnt, theDmx->chkt_cnt, ESP.getFreeHeap()
                /* ,lora_sent_pkts, lora_rcvd_pkts, lora_pps */);
    next_lora_log = millis() + LORA_LOG_INTERVAL;
  }
  
  if (next_serial_ts < millis()) {
    Serial.printf("-- %s F=%d E=%d C=%d |dmxt|=%d |chkt|=%d |heap|=%d\r\n",
                  _ts(),
                  theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt,
                  theDmx->dmxt_cnt, theDmx->chkt_cnt, ESP.getFreeHeap()
                  /* ,lora_sent_pkts, lora_rcvd_pkts, lora_pps */);

    next_serial_ts = millis() + SERIAL_TS_INTERVAL;
  }
}

// ---------------------------------------------------------------------------

void setup()
{
  char *msg;

  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.println("** Welcome to the tinySSB virtual pub "
                 "running on a " DEVICE_MAKE);
  Serial.println("** compiled " __DATE__ ", " __TIME__ UTC_OFFSET);

  esp_efuse_mac_get_default(my_mac);
#if !defined(HAS_UDP)  // in case of no Wifi: display BT mac addr, instead
  my_mac[5] += 2;
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.0/api-reference/system/base_mac_address.html
#endif
  
  // #if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2));
  Serial.printf("** this is node %s\r\n\r\n", ssid);
  // #endif

  hw_init();

#if defined(TINYSSB_BOARD_HELTEC)
  theUI    = new UI_Heltec_Class();
#elif defined(TINYSSB_BOARD_T5GRAY)
  theUI    = new UI_T5gray_Class();
#elif defined(TINYSSB_BOARD_TBEAM)
  theUI    = new UI_TBeam_Class();
#elif defined(TINYSSB_BOARD_TDECK)
  theUI    = new UI_TDeck_Class();
#elif defined(TINYSSB_BOARD_TWRIST)
  theUI    = new UI_TWrist_Class();
#endif
  // theUI->show_node_name(ssid);
  theUI->spinner(true);

  if (!MyFS.begin(true)) {
    msg = "LittleFS Mount Failed, partition was reformatted";
    theUI->show_boot_msg(msg);
    Serial.println(msg);
    // return;
  } else
    theUI->show_boot_msg("mounted LittleFS");
  // MyFS.format(); // uncomment and run once after a change in partition size

  MyFS.mkdir(FEED_DIR);
  Serial.printf("LittleFS: %d total bytes, %d used\r\n",
                MyFS.totalBytes(), MyFS.usedBytes());

  theUI->show_boot_msg("load config");
  the_config = config_load();

#ifdef TINYSSB_BOARD_TDECK
  struct bipf_s k = { BIPF_STRING, 9, {.str = "lora_plan"} };
  struct bipf_s *v = bipf_dict_getref(the_config, &k);

  if (v != NULL && v->typ == BIPF_STRING &&
                                           strncmp("US915.b", v->u.str, v->cnt)) {
    bipf_dict_set(the_config, bipf_mkString("lora_plan"),
                  bipf_mkString("US915.b"));
    config_save(the_config);
  }
#endif

  // FIXME: we should not print the mgmt signing key to the console ?
  Serial.printf("tinySSB config is %s\r\n",
                bipf2String(the_config, "\r\n", 1).c_str());
  Serial.println();

  theUI->show_boot_msg("setup log files");
  msg = "** starting";
  lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_APPEND);
  lora_log_wr(msg);
  peers_log = MyFS.open(PEERS_DATA_FILENAME, FILE_APPEND);
  peers_log_wr(msg);
  
#ifdef USE_LORA_LIB
  theUI->show_boot_msg("init LoRa");
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);  
  if (!LoRa.begin(the_lora_config->fr)) {
    msg = "  LoRa init failed!";
    theUI->show_boot_msg(msg);
    while (1) {
      Serial.println(msg);
      delay(2000);
    }
  }
  // LoRa.setTxPower(the_lora_config->tx, RF_PACONFIG_PASELECT_PABOOST);
#endif

  theUI->show_boot_msg("init BLE, WIFI");
  io_init(); // network interfaces

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
  }

  theUI->show_boot_msg("load feed data ...");
  theRepo->load();
  Serial.printf("\r\n   Repo: %d feeds, %d entries, %d chunks\r\n",
                theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt);
  // theUI->show_repo_stats(theRepo->rplca_cnt,
  //                      theRepo->entry_cnt, theRepo->chunk_cnt, 1);
  
  // listDir(MyFS, "/", 2); // FEED_DIR, 2);
  theUI->show_boot_msg("load app data ...");
  // the_TVA_app = new App_TVA_Class(posts);
  // the_TVA_app->restream();

  setup_la_wifi_web();

  theUI->spinner(false);
  theUI->buzz();

  msg = "end of setup\n";
  theUI->show_boot_msg(msg);
  Serial.println(msg);
  delay(1000);

  theUI->refresh();
}


void loop()
{
  io_loop();        // check for received packets
  io_proc();        // process received packets
  theSched->tick(); // send pending packets

  theUI->loop();

  if (Serial.available())
    cmd_rx(Serial.readString());

  time_stamp();
  delay(5);
}

// eof
