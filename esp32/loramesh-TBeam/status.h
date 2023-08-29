// status.h

#ifndef _INCLUDE_STATUS_H
#define _INCLUDE_STATUS_H

struct peer_s {
  char id[5];
  long when;
  int rssi;
  float snr;
};
#define MAX_PEERS 4


class StatusClass {

public:
  void init();

  // repo screen:
  void set_time(char *s);
  void set_wifi_peers(int n);
  void set_ble_peers(int n);
  void advance_lora_wheel();

  // peers screen:
  void heard_peer(char *id, int rssi, float snr);

  // for all screens:
  void refresh_screen(int n);
  void to_next_screen();

private:
  char curr_screen;

  // repo
  char *r_time;
  bool r_gps_valid;
  int r_wifi_peers, r_ble_peers;
  int r_f, r_e, r_c, r_free_space;
  int r_lora_wheel;

  struct peer_s peers[MAX_PEERS];
};

enum {
  SCREEN_INIT,
  SCREEN_REPO,
  SCREEN_PEERS,
  SCREEN_OFF
};

#endif
