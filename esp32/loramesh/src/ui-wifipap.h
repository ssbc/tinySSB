// ui-wifipap.h

#ifndef _INCLUDE_UI_WIFIPAP_H
#define _INCLUDE_UI_WIFIPAP_H

// Heltec Wifi-Paper

#include "lib/tinySSBlib.h"

class UI_WifiPaper_Class: public UIClass {

  using UIClass::UIClass;

public:

  enum {
    SCREEN_SPLASH,
    SCREEN_NODE,
    SCREEN_PEERS,
    SCREEN_LORA,
    SCREEN_OFF
  };
  
  UI_WifiPaper_Class();
  void loop() override; // for screen animations
  void refresh() override;

  void to_next_screen();
  void refresh_screen(int n);
  void show_boot_msg(char *s);

  char curr_screen;
private:
  // repo:
  int r_f, r_e, r_c, r_free_space;
  // char *r_time;
  // bool r_gps_valid;
  int lora_wheel;
  char *wheel;
};

#endif // _INCLUDE_UI_WIFIPAP_H
