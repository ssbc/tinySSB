// status.cpp

#include "config.h"

static const char *wheel[] = {"\\", "|", "/", "-"};
extern char my_ssid[];


void StatusClass::init()
{
  r_time = NULL;

#if defined(HAS_OLED) && defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  theDisplay.init();
  theDisplay.flipScreenVertically();
  theDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
#endif

  curr_screen = SCREEN_INIT;
  refresh_screen(curr_screen);
}


static void right_aligned(int cnt, char c, int y)
{
  char buf[20];
  sprintf(buf, "%d %c", cnt, c);
  int w = theDisplay.getStringWidth(buf);
  theDisplay.drawString(128-w, y, buf);
}


void StatusClass::set_time(char *cp)
{
  if (r_time)
    free(r_time);
  r_time = strdup(cp);
}


void StatusClass::set_wifi_peers(int n)
{
  r_wifi_peers = n;
}


void StatusClass::set_ble_peers(int n)
{
  r_ble_peers = n;
}


void StatusClass::advance_lora_wheel()
{
  r_lora_wheel++;
}


void StatusClass::heard_peer(char *id, int rssi, float snr)
{
  long oldest = 0;
  int i, oldest_i = -1;

  for (i = 0; i < MAX_PEERS; i++) {
    if (!strcmp(peers[i].id, id) || peers[i].id[0] == '\0')
      break;
    if (oldest == 0 || oldest < peers[i].when) {
      oldest = peers[i].when;
      oldest_i = i;
    }
  }
  if (i >= MAX_PEERS) {
    if (oldest_i == -1) // this should not happen
      return;
    i = oldest_i;
  }
  strcpy(peers[i].id, id);
  peers[i].when = millis();
  peers[i].rssi = rssi;
  peers[i].snr = snr;
  
  refresh_screen(SCREEN_PEERS);
}


void StatusClass::refresh_screen(int scr)
{
#if !defined(HAS_OLED)
  return;
#else
  if (scr != curr_screen)
      return;
  if (scr == SCREEN_OFF) {
    theDisplay.displayOff();
    return;
  }
  theDisplay.displayOn();
  theDisplay.clear();

  if (scr == SCREEN_INIT) {
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0 , 0, "SSB.virt.lora.pub");
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0 , 17, __DATE__ " " __TIME__ UTC_OFFSET);

    int f = the_lora_config->fr / 10000;
    char fr[30];
    sprintf(fr, "%d.%02d MHz", f/100, f%100);
    theDisplay.setFont(ArialMT_Plain_24);
    theDisplay.drawString(0, 30, fr);
    sprintf(fr, "%s    SF%d BW%d", the_lora_config->plan,
      the_lora_config->sf, (int)(the_lora_config->bw/1000));
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 54, fr);
  } else if (scr == SCREEN_REPO) {
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 3, tSSB_WIFI_SSID "-");
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(42, 0, my_ssid+8);
    theDisplay.setFont(ArialMT_Plain_10);

    if (r_time)
      theDisplay.drawString(0, 18, r_time);

    char stat_line[30];
    char gps_synced = 0;
# if !defined(NO_GPS)
    gps_synced = gps.location.isValid() ? 1 : 0;
#endif
    sprintf(stat_line, "W:%d E:%d G:%d L:%s",
            r_wifi_peers, r_ble_peers, r_gps_valid, wheel[r_lora_wheel % 4]);
    theDisplay.drawString(0, 30, stat_line);

    theDisplay.setFont(ArialMT_Plain_16);
    right_aligned(repo->rplca_cnt, 'F', 0); 
    right_aligned(repo->entry_cnt, 'E', 22); 
    right_aligned(repo->chunk_cnt, 'C', 44); 

    int total = MyFS.totalBytes();
    int avail = total - MyFS.usedBytes();
    char buf[10];
    sprintf(buf, "%2d%% free", avail / (total/100));
    theDisplay.drawString(0, 44, buf);
  } else if (scr == SCREEN_PEERS) {
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString( 0, 0, "last");
    theDisplay.drawString(35, 0, "peer");
    theDisplay.drawString(65, 0, "rssi");
    theDisplay.drawString(95, 0, "snr");
    int y = 14;
    // theDisplay.setFont(ArialMT_Plain_10);
    long now = millis();
    for (int i = 0; i < MAX_PEERS; i++) {
      if (peers[i].id[0] == '\0')
        continue;
      theDisplay.drawString( 0, y, String((peers[i].when - now)/1000));
      theDisplay.drawString(35, y, peers[i].id);
      theDisplay.drawString(65, y, String(peers[i].rssi));
      theDisplay.drawString(95, y, String(peers[i].snr));
      y += 12;
    }
  }

  theDisplay.display();
#endif // HAS_OLED
}


void StatusClass::to_next_screen()
{
  curr_screen = (curr_screen + 1) % 4;
  refresh_screen(curr_screen);
}


// eof
