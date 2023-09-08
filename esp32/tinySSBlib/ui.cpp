// tinySSBlib/ui.cpp

#include "tinySSBlib.h"

UIClass::~UIClass() {}


void UIClass::show_node_name(char *s)
{ if (node_name) free(node_name); node_name = strdup(s); }

void UIClass::show_time(char *s)
{ if (time) free(time); time = strdup(s); }

void UIClass::show_gps(float lon, float lat, float ele)
{ gps_lon = lon, gps_lat = lat, gps_ele = ele; }

void UIClass::show_repo_stats(int f, int e, int c, int freePercent)
{ f_cnt = f, e_cnt = e, c_cnt = c; }

void UIClass::show_ble_peers(int n)
{ ble_cnt = n; }

void UIClass::show_wifi_peers(int n)
{ wifi_cnt = n; }

void UIClass::show_lora_specs(char *profile, long fr, int bw, int sf)
{
  if (lora_profile)
    free(lora_profile);
  lora_profile = profile ? strdup(profile) : NULL;
  lora_fr = fr, lora_bw = bw, lora_sf = sf;
}

void UIClass::heard_peer(char *id, int rssi, float snr)
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
}

// eof