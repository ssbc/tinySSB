// loramesh-TBeam.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// -----------------------------------------------------------------------------

#include "config.h"  // all definitions and header files
#include "hw_setup.h"

// have Serial be initialized early i.e., before other global objects
class Dummy { public: Dummy(); };
Dummy::Dummy() { Serial.begin(BAUD_RATE); }
Dummy *d = new Dummy();

DmxClass    *dmx       = new DmxClass();
Repo2Class  *repo      = new Repo2Class();
GOsetClass  *theGOset  = new GOsetClass();
StatusClass *theStatus = new StatusClass();

// our own local code:
#include "bipf.h"
#include "kiss.h"
#include "node.h"
#include "ed25519.h"
#include "mgmt.h"
#include "peers.h"
#include "config.h"

struct bipf_s *the_config;
struct lora_config_s *the_lora_config;

int ble_clients = 0;

#if defined(HAS_OLED) && defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  SSD1306 theDisplay(0x3c, 21, 22); // lilygo t-beam
#endif

/* FTP server would be neat:
// #define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_LITTLEFS
#define DEFAULT_FTP_SERVER_NETWORK_TYPE_ESP32           NETWORK_ESP32
#define DEFAULT_STORAGE_TYPE_ESP32                      STORAGE_LITTLEFS
#include <FtpServer.h>
FtpServer ftpSrv;
*/

// char lora_line[80];
char loc_line[80];
char goset_line[80];
char refresh = 1;

int old_gps_sec, old_goset_c, old_goset_n, old_goset_len;
int old_repo_sum;
int lora_cnt = 0;
int lora_bad_crc = 0;

File lora_log;
unsigned long int next_log_flush;

File peer_log;

#include "cmd.h"

void theGOset_rx(unsigned char *pkt, int len, unsigned char *aux,
                 struct face_s *f)
{
  theGOset->rx(pkt, len, aux, f);
}

// ----------------------------------------------------------------------------

void draw_lora_plan()
{
#if defined(HAS_OLED)
  theDisplay.clear();

  theDisplay.clear();
  theDisplay.setFont(ArialMT_Plain_24);
  theDisplay.drawString(0, 0, the_lora_config->plan);
  
  char s[30];
  theDisplay.setFont(ArialMT_Plain_16);
  int f = the_lora_config->fr / 10000;
  sprintf(s, "fr=%d.%02d MHz", f/100, f%100);
  theDisplay.drawString(0, 28, s);
  sprintf(s, "sf=%d bw=%d kHz",
          the_lora_config->sf, (int)(the_lora_config->bw/1000));
  theDisplay.drawString(0, 48, s);

  theDisplay.display();
#endif
}

// ----------------------------------------------------------------------------

Button2 userButton;
char lora_selection; // true if lora plan selection ongoing, else do_io
long selection_timeout;

void clicked(Button2& btn) {
  // Serial.println("click");
  selection_timeout = millis() + 10000;

  if (lora_selection) { // cycle through the choices
    int i = the_lora_config - lora_configs;
    i = (i+1) % lora_configs_size;
    the_lora_config = lora_configs + i;
  } else
    theStatus->to_next_screen();
}

void long_clicked(Button2& btn) {
  // Serial.println("long_click");
  selection_timeout = millis() + 10000;

  if (lora_selection) { // this is our choice
    struct bipf_s *v = bipf_mkString(the_lora_config->plan);
    bipf_dict_set(the_config, bipf_mkString("lora_plan"), v);
    config_save(the_config);

    char s[30];
    theDisplay.clear();
    theDisplay.setFont(ArialMT_Plain_24);
    sprintf(s, ">> %s", the_lora_config->plan);
    theDisplay.drawString(0, 10, s);
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0, 46, "rebooting now");
    theDisplay.display();

    delay(3000);
    lora_log.printf(">> reboot cmd\n");
    lora_log.close();
    esp_restart();
  } else
    lora_selection = 1 - lora_selection;
}

// ----------------------------------------------------------------------------

void setup()
{
  hw_setup();

  // pinMode(BUTTON_PIN, INPUT);
  userButton.begin(BUTTON_PIN);
  userButton.setLongClickTime(2000);
  userButton.setClickHandler(clicked);
  userButton.setLongClickDetectedHandler(long_clicked);

  io_init();

  Serial.printf("File system: %d total bytes, %d used\r\n",
                MyFS.totalBytes(), MyFS.usedBytes());
  MyFS.mkdir(FEED_DIR);
  listDir(MyFS, FEED_DIR, 0);
  // ftpSrv.begin(".",".");

  Serial.println();
  // theGOset = GOsetClass();
  unsigned char h[32];
  crypto_hash_sha256(h, (unsigned char*) GOSET_DMX_STR, strlen(GOSET_DMX_STR));
  memcpy(dmx->goset_dmx, h, DMX_LEN);
  dmx->arm_dmx(dmx->goset_dmx, theGOset_rx, NULL);
  Serial.printf("   DMX for GOST is %s\r\n", to_hex(dmx->goset_dmx, 7, 0));

  mgmt_setup();
  peer_init();

  // cmd_rx("r");
  repo->load();

  // strcpy(lora_line, "?");
  theStatus->set_time("--no time signal--");
  strcpy(loc_line, "?");
  strcpy(goset_line, "?");

#if defined(LORA_LOG)
  lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_APPEND);
  lora_log.printf("reboot\n");
#if defined(AXP_DEBUG)
  unsigned long int m = millis();
  lora_log.printf("t=%d.%03d battery=%.04gV\n", m/1000, m%1000,
                                                getBattVoltage());
  next_log_battery = m + LOG_BATTERY_INTERVAL;
#endif
  lora_log.printf("millis,batt,utc,mac,lat,lon,ele,plen,prssi,psnr,pfe,rssi\n");
  next_log_flush = millis() + LOG_FLUSH_INTERVAL;
  Serial.printf("\r\nlength of %s: %d bytes\r\n", LORA_LOG_FILENAME, lora_log.size());
#endif // LORA_LOG
  Serial.printf("length of %s: %d bytes\r\n", PEERS_FILENAME, peer_log.size());

  

  Serial.printf("\r\nHeap: %d total, %d free, %d min, %d maxAlloc\r\n",
                 ESP.getHeapSize(), ESP.getFreeHeap(),
                 ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());

  Serial.println("\r\ninit done, starting loop now. Type '?' for list of commands\r\n");

  delay(1500); // keep the screen for some time so the display headline can be read ..
  theStatus->to_next_screen();

  // cpu_set_slow();

/*
  Serial.println("LIGHT SLEEP ENABLED FOR 5secs");
  delay(100);
 
  esp_sleep_enable_timer_wakeup(5 * 1000 * 1000);
  esp_light_sleep_start();
 
  Serial.println();
  Serial.println("LIGHT SLEEP WAKE UP");
*/
}

// ----------------------------------------------------------------------

int incoming(struct face_s *f, unsigned char *pkt, int len, int has_crc)
{
  if (len <= (DMX_LEN + sizeof(uint32_t)) || (has_crc && crc_check(pkt, len) != 0)) {
    Serial.println(String("Bad CRC for face ") + f->name + String(" pkt=") + to_hex(pkt, len, 0));
    lora_bad_crc++;
    return -1;
  }
  if (has_crc) {
    // Serial.println("CRC OK");
    len -= sizeof(uint32_t);
  }
  if (!dmx->on_rx(pkt, len, f))
    return 0;
  Serial.println(String("   unknown DMX ") + to_hex(pkt, DMX_LEN, 0));
  return -1;
}

/*
void right_aligned(int cnt, char c, int y)
{
#if defined(HAS_OLED)
  char buf[20];
  sprintf(buf, "%d %c", cnt, c);
  int w = theDisplay.getStringWidth(buf);
  theDisplay.drawString(128-w, y, buf);
#endif
}

const char *wheel[] = {"\\", "|", "/", "-"};
int spin;
*/

// ----------------------------------------------------------------------

void do_selection()
{
  static char blink;
  static long next, timeout;

  if (millis() < next)
    return;
  next = millis() + 100;
  blink = 1 - blink;

  if (selection_timeout < millis()) {
    lora_selection = 0;
    refresh = 1;
    return;
  }

#if defined(HAS_OLED)
  if (blink) {
    theDisplay.clear();
    theDisplay.display();
  } else {
    draw_lora_plan();
    next += 300;
  }
#endif
}


void do_io()
{
  unsigned char pkt_buf[200], *cp;
  int packetSize, pkt_len;

#if !defined(NO_WIFI)
  if (WiFi.softAPgetStationNum() != wifi_clients) {
    theStatus->set_wifi_peers(WiFi.softAPgetStationNum());
    refresh = 1;
  }
#endif
  
#if defined(MAIN_BLEDevice_H_)
  if (bleDeviceConnected != ble_clients) {
    theStatus->set_ble_peers(bleDeviceConnected);
    refresh = 1;
  }
#endif

  userButton.loop();
  io_dequeue();
  
  theGOset->tick();
  node_tick();
  // mgmt_tick();
  peer_tick();

  if (Serial.available())
    cmd_rx(Serial.readString());

#if defined(HAS_LORA)
  fishForNewLoRaPkt();
  pkt_len = lora_get_pkt(pkt_buf);
  if (pkt_len > 0) {
    lora_cnt++;
    theStatus->advance_lora_wheel();
    // Serial.printf("Rcv LoRa %dB\r\n", pkt_len);
    incoming(&lora_face, pkt_buf, pkt_len, 1);
    // sprintf(lora_line, "LoRa %d/%d: %dB, rssi=%d", lora_cnt, lora_bad_crc, pkt_len, LoRa.packetRssi());
    refresh = 1;
  }

#ifdef OLD_LORA
  packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    pkt_len = 0;
    while (packetSize-- > 0) {
      unsigned char c = LoRa.read();
      if (pkt_len < sizeof(pkt_buf))
        pkt_buf[pkt_len++] = c;
    }
#if defined (LORA_LOG)
    {
      unsigned long int m = millis();
      long pfe = 0;
      int rssi = 0;

#if defined(ARDUINO_heltec_wifi_lora_32_V2)
      lora_log.printf("%d.%03d,0000-00-00T00:00:00Z,%s,0,0,0",
                      m/1000, m%1000,
                      to_hex(my_mac,6,1));
#else
# if defined(HAS_GPS)
      lora_log.printf("%d.%03d,%04d-%02d-%02dT%02d:%02d:%02dZ,%s",
                      m/1000, m%1000,
                      gps.date.year(), gps.date.month(), gps.date.day(),
                      gps.time.hour(), gps.time.minute(), gps.time.second(),
                      // gps.time.centisecond(),
                      to_hex(my_mac,6,1));
      if (gps.location.isValid())
        lora_log.printf(",%.8g,%.8g,%g", gps.location.lat(),
                        gps.location.lng(), gps.altitude.meters());
      else
        lora_log.printf(",0,0,0");
#endif
      pfe   = LoRa.packetFrequencyError();
      rssi   = LoRa.rssi();
#endif

      lora_log.printf(",%d,%d,%g,%ld,%d\n", pkt_len, lora_prssi, lora_psnr,
                      pfe, rssi);
      if (millis() > next_log_flush) {
        lora_log.flush();
        next_log_flush = millis() + LOG_FLUSH_INTERVAL;
      }
    }
#endif
    lora_cnt++;
    theStatus->advance_lora_wheel();
    Serial.printf("<< rcv LoRa %dB\r\n", pkt_len);
    incoming(&lora_face, pkt_buf, pkt_len, 1);
    // sprintf(lora_line, "LoRa %d/%d: %dB, rssi=%d", lora_cnt, lora_bad_crc, pkt_len, LoRa.packetRssi());
    refresh = 1;
  }
#endif // OLD_LORA
#endif // HAS_LORA

#if !defined(NO_WIFI)
  packetSize = udp.parsePacket();
  if (packetSize) {
    // Serial.print("UDP " + String(packetSize) + "B from "); 
    // Serial.print(udp.remoteIP());
    // Serial.println("/" + String(udp.remotePort()));
    pkt_len = udp.read(pkt_buf, sizeof(pkt_buf));
    incoming(&udp_face, pkt_buf, pkt_len, 1);
  }
#endif

#if !defined(NO_BT)
  packetSize = kiss_read(BT, &bt_kiss);
  if (packetSize > 0) {
    incoming(&bt_face, bt_kiss.buf, packetSize, 1);
  }
#endif

#if defined(MAIN_BLEDevice_H_) && !defined(NO_BLE)
  cp = ble_fetch_received();
  if (cp != NULL) {
    incoming(&ble_face, cp+1, *cp, 0);
  }
#endif

#if defined(LORA_LOG)

  if (millis() > next_log_battery) {
    unsigned long int now = millis();
    lora_log.printf("t=%d.%03d ", now/1000, now%1000);
# if defined(AXP_DEBUG)
    lora_log.printf("battery=%.04gV ", getBattVoltage());
# endif
    lora_log.printf("|dmxt|=%d |blbt|=%d |feeds|=%d |entries|=%d |chunks|=%d |freeHeap|=%dXX\r\n",
                dmx->dmxt_cnt, dmx->blbt_cnt, repo->rplca_cnt, repo->entry_cnt, repo->chunk_cnt, ESP.getFreeHeap());
    next_log_battery = millis() + LOG_BATTERY_INTERVAL;
  }

# if defined(HAS_GPS)
  while (GPS.available())
    gps.encode(GPS.read());
  if (gps.time.second() != old_gps_sec) {
    char time_line[80];
    old_gps_sec = gps.time.second();
    sprintf(time_line, "%02d:%02d:%02d utc", gps.time.hour(), gps.time.minute(),
                                      old_gps_sec);
    theStatus->set_time(time_line);
    /*
    if (gps.location.isValid())
      sprintf(loc_line, "%.8g@%.8g@%g/%d", gps.location.lat(), gps.location.lng(),
                                      gps.altitude.meters(), gps.satellites.value());
    else
      strcpy(loc_line, "-@-@-/-");
    */
    refresh = 1;
  }
# endif
  if (old_goset_c != theGOset->pending_c_cnt || 
      old_goset_n != theGOset->pending_n_cnt ||
      old_goset_len != theGOset->goset_len) {
    old_goset_c = theGOset->pending_c_cnt;
    old_goset_n = theGOset->pending_n_cnt;
    old_goset_len = theGOset->goset_len;
    sprintf(goset_line, "GOS: len=%d, pn=%d, pc=%d", old_goset_len, old_goset_n, old_goset_c);
    refresh = 1;
  }
#endif // LORA_LOG

  int sum = repo->rplca_cnt + repo->entry_cnt + repo->chunk_cnt;
  if (sum != old_repo_sum) {
    old_repo_sum = sum;
    refresh = 1;
  }

#if defined(HAS_OLED)
  if (refresh) {
    refresh = 0;
    theStatus->refresh_screen(SCREEN_REPO);
/*
  send new BLE status ... stat_line see status.cpp

#if defined(MAIN_BLEDevice_H_)
#if defined(ARDUINO_heltec_wifi_lora_32_V2)
    sprintf(stat_line + strlen(stat_line)-1, "%d", lora_cnt);
#else
    sprintf(stat_line + strlen(stat_line)-1, "%d, batt:%.04g",
            lora_cnt, getBattVoltage());
#endif
    ble_send_stats((unsigned char*) stat_line, strlen(stat_line));
#endif
*/
  }
#endif // HAS_OLED
}

// ----------------------------------------------------------------------

void loop()
{
  userButton.loop();

  if (lora_selection)
    do_selection();
  else
    do_io();

  delay(10);
  // ftpSrv.handleFTP();
}

// eof
