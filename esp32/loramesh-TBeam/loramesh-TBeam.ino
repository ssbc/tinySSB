// loramesh-TBeam.ino

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// -----------------------------------------------------------------------------

#include "config.h"  // all definitions and header files
#include "hw_setup.h"

// have Serial be initialized early i.e., before other global objects
class Dummy { public: Dummy(); };
Dummy::Dummy() { Serial.begin(BAUD_RATE); }
Dummy *d = new Dummy();

DmxClass   *dmx      = new DmxClass();
Repo2Class *repo     = new Repo2Class();
GOsetClass *theGOset = new GOsetClass();

// our own local code:
#include "bipf.h"
#include "kiss.h"
#include "node.h"
#include "ed25519.h"
#include "mgmt.h"
#include "config.h"

struct bipf_s *the_config;
struct lora_config_s *the_lora_config;

int ble_clients = 0;

#if !defined(NO_OLED) && !defined(WIFI_LoRa_32_V2) && !defined(WIFI_LORA_32_V2)
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
char time_line[80];
char loc_line[80];
char goset_line[80];
char refresh = 1;

int old_gps_sec, old_goset_c, old_goset_n, old_goset_len;
int old_repo_sum;
int lora_cnt = 0;
int lora_bad_crc = 0;

File lora_log;
unsigned long int next_log_flush;


#include "cmd.h"

void theGOset_rx(unsigned char *pkt, int len, unsigned char *aux,
                 struct face_s *f)
{
  theGOset->rx(pkt, len, aux, f);
}

// ----------------------------------------------------------------------------

void setup()
{
  hw_setup();

#if !defined(NO_OLED)
  theDisplay.setFont(ArialMT_Plain_16);
  theDisplay.drawString(0 , 0, "SSB.virt.lora.pub");
  theDisplay.setFont(ArialMT_Plain_10);
  theDisplay.drawString(0 , 17, __DATE__ " " __TIME__);

  int f = the_lora_config->fr / 10000;
  char fr[30];
  sprintf(fr, "%d.%02d MHz", f/100, f%100);
  theDisplay.setFont(ArialMT_Plain_24);
  theDisplay.drawString(0, 30, fr);
  sprintf(fr, "%s    SF%dBW%d", the_lora_config->plan,
    the_lora_config->sf, (int)(the_lora_config->bw/1000));
  theDisplay.setFont(ArialMT_Plain_10);
  theDisplay.drawString(0, 54, fr);

  theDisplay.display();
#endif
  
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

  // cmd_rx("r");
  repo->load();

  // strcpy(lora_line, "?");
  strcpy(time_line, "?");
  strcpy(loc_line, "?");
  strcpy(goset_line, "?");

#if defined(LORA_LOG)
  lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_APPEND);
  lora_log.printf("reboot\n");
#if defined(AXP_DEBUG)
  unsigned long int m = millis();
  lora_log.printf("t=%d.%03d battery=%.04gV\n", m/1000, m%1000,
                                                axp.getBattVoltage()/1000);
  next_log_battery = m + LOG_BATTERY_INTERVAL;
#endif
  lora_log.printf("millis,batt,utc,mac,lat,lon,ele,plen,prssi,psnr,pfe,rssi\n");
  next_log_flush = millis() + LOG_FLUSH_INTERVAL;
  Serial.printf("\r\nlength of %s: %d bytes\r\n", LORA_LOG_FILENAME, lora_log.size());
#endif // LORA_LOG

  Serial.printf("\r\nHeap: %d total, %d free, %d min, %d maxAlloc\r\n",
                 ESP.getHeapSize(), ESP.getFreeHeap(),
                 ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());

  Serial.println("\r\ninit done, starting loop now. Type '?' for list of commands\r\n");

  delay(1500); // keep the screen for some time so the display headline can be read ..
  // OLED_toggle(); // default is OLED off, use button to switch on

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

void right_aligned(int cnt, char c, int y)
{
#if !defined(NO_OLED)
  char buf[20];
  sprintf(buf, "%d %c", cnt, c);
  int w = theDisplay.getStringWidth(buf);
  theDisplay.drawString(128-w, y, buf);
#endif
}

const char *wheel[] = {"\\", "|", "/", "-"};
int spin;

// ----------------------------------------------------------------------

void loop()
{
  unsigned char pkt_buf[200], *cp;
  int packetSize, pkt_len;

#if !defined(NO_WIFI)
  if (WiFi.softAPgetStationNum() != wifi_clients) {
    wifi_clients = WiFi.softAPgetStationNum();
    refresh = 1;
  }
#endif
  
#if defined(MAIN_BLEDevice_H_)
  if (bleDeviceConnected != ble_clients) {
    ble_clients = bleDeviceConnected;
    refresh = 1;
  }
#endif

  userButton.loop();
  io_dequeue();
  theGOset->tick();
  node_tick();
  // mgmt_tick();

  if (Serial.available())
    cmd_rx(Serial.readString());

#if defined(HAS_LORA)
  fishForNewLoRaPkt();
  pkt_len = lora_get_pkt(pkt_buf);
  if (pkt_len > 0) {
    lora_cnt++;
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

#if defined(WIFI_LoRa_32_V2) || defined(WIFI_LORA_32_V2)
      lora_log.printf("%d.%03d,0000-00-00T00:00:00Z,%s,0,0,0",
                      m/1000, m%1000,
                      to_hex(my_mac,6,1));
#else
# if !defined(NO_GPS)
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

      int prssi  = LoRa.packetRssi();
      float psnr = LoRa.packetSnr();

      lora_log.printf(",%d,%d,%g,%ld,%d\n", pkt_len, prssi, psnr, pfe, rssi);
      if (millis() > next_log_flush) {
        lora_log.flush();
        next_log_flush = millis() + LOG_FLUSH_INTERVAL;
      }
    }
#endif
    lora_cnt++;
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
    lora_log.printf("battery=%.04gV ", axp.getBattVoltage()/1000);
# endif
    lora_log.printf("|dmxt|=%d |blbt|=%d |feeds|=%d |entries|=%d |chunks|=%d |freeHeap|=%dXX\r\n",
                dmx->dmxt_cnt, dmx->blbt_cnt, repo->rplca_cnt, repo->entry_cnt, repo->chunk_cnt, ESP.getFreeHeap());
    next_log_battery = millis() + LOG_BATTERY_INTERVAL;
  }

# if !defined(NO_GPS)
  while (GPS.available())
    gps.encode(GPS.read());
  if (gps.time.second() != old_gps_sec) {
    old_gps_sec = gps.time.second();
    sprintf(time_line, "%02d:%02d:%02d utc", gps.time.hour(), gps.time.minute(),
                                      old_gps_sec);
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

#if !defined(NO_OLED)
  if (refresh) {
    theDisplay.clear();

    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 3, tSSB_WIFI_SSID "-");
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(42, 0, ssid+8);
    theDisplay.setFont(ArialMT_Plain_10);
    
    theDisplay.drawString(0, 18, time_line);
    // theDisplay.drawString(0, 24, goset_line);
    char stat_line[30];
    char gps_synced = 0;
# if !defined(NO_GPS)
    gps_synced = gps.location.isValid() ? 1 : 0;
#endif
    sprintf(stat_line, "W:%d E:%d G:%d L:%s",
            wifi_clients, ble_clients, gps_synced, wheel[lora_cnt % 4]);
    theDisplay.drawString(0, 30, stat_line);

#if defined(MAIN_BLEDevice_H_)
#if defined(WIFI_LoRa_32_V2) || defined(WIFI_LORA_32_V2)
    sprintf(stat_line + strlen(stat_line)-1, "%d", lora_cnt);
#else
    sprintf(stat_line + strlen(stat_line)-1, "%d, batt:%.04g",
            lora_cnt, axp.getBattVoltage()/1000);
#endif
    ble_send_stats((unsigned char*) stat_line, strlen(stat_line));
#endif

    theDisplay.setFont(ArialMT_Plain_16);
    right_aligned(repo->rplca_cnt, 'F', 0); 
    right_aligned(repo->entry_cnt, 'E', 22); 
    right_aligned(repo->chunk_cnt, 'C', 44); 

    int total = MyFS.totalBytes();
    int avail = total - MyFS.usedBytes();
    char buf[10];
    sprintf(buf, "%2d%% free", avail / (total/100));
    theDisplay.drawString(0, 44, buf);

    // theDisplay.drawString(0, 12, wheel[spin++ % 4]);     // lora_line
    theDisplay.display();
    refresh = 0;
  }
#endif // NO_OLED

  delay(10);

  // ftpSrv.handleFTP();
}

// eof
