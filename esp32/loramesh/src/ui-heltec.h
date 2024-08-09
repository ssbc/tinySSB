// ui-heltec.h

#ifndef _INCLUDE_UI_HELTEC_H
#define _INCLUDE_UI_HELTEC_H

#include "lib/tinySSBlib.h"

class UI_Heltec_Class: public UIClass {

  using UIClass::UIClass;

public:

  enum {
    SCREEN_SPLASH,
    SCREEN_NODE,
    SCREEN_PEERS,
    SCREEN_LORA,
    SCREEN_OFF
  };
  
  UI_Heltec_Class();
  void loop() override; // for screen animations
  void refresh() override;

  void to_next_screen();
  void refresh_screen(int n);
  void show_boot_msg(char *s);
  void lora_advance_wheel();

  char curr_screen;
private:
  // repo:
  int r_f, r_e, r_c, r_free_space;
  // char *r_time;
  // bool r_gps_valid;
  int lora_wheel;
  char *wheel;
};

#endif // _INCLUDE_UI_HELTEC_H
