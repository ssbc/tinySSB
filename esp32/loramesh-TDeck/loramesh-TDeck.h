// loramesh_TDeck.h

#include "src/lib/tinySSBlib.h"
#include "src/ui-tdeck.h"
#include "src/ui-twrist.h"

#if !defined(UTC_OFFSET)
# define UTC_OFFSET ""
#endif

// include files with actual code and data structures:
#include "src/lib/node.h"
#if defined(HAS_BT)
# include "src/lib/kiss.h"
#endif

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

void setup()
{
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
  
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2));
  Serial.printf("** this is node %s\r\n\r\n", ssid);
#endif
  
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

#if defined(TINYSSB_BOARD_TDECK)  
  theUI    = new UI_TDeck_Class();
#elif defined(TINYSSB_BOARD_TWRIST)
  theUI    = new UI_TWrist_Class();
#endif
  // theUI->show_node_name(ssid);
  theUI->spinner(true);

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

  theRepo->load();
  Serial.printf("\r\n   Repo: %d feeds, %d entries, %d chunks\r\n",
                theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt);
  // theUI->show_repo_stats(theRepo->rplca_cnt,
  //                      theRepo->entry_cnt, theRepo->chunk_cnt, 1);
  
  // listDir(MyFS, "/", 2); // FEED_DIR, 2);
  // the_TVA_app = new App_TVA_Class(posts);
  // the_TVA_app->restream();

  Serial.println("end of setup\n");
  theUI->spinner(false);
  theUI->buzz();
  theUI->refresh();
}


void loop()
{
  io_loop();        // check for received packets
  io_proc();        // process received packets
  theSched->tick(); // send pending packets

  theUI->loop();

  delay(5);
}

// eof
