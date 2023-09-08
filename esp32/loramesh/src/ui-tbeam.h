// ui-tbeam.h

#ifndef _INCLUDE_UI_TBEAM_H
#define _INCLUDE_UI_TBEAM_H

#include "lib/tinySSBlib.h"
#include "const-tbeam.h"

enum {
  SCREEN_SPLASH,
  SCREEN_NODE,
  SCREEN_LORA,
  SCREEN_PEERS,
  SCREEN_OFF
};


class UI_TBeam_Class: public UIClass {

  using UIClass::UIClass;

public:
  UI_TBeam_Class();
  void loop() override; // for screen animations
  void refresh() override;

  void to_next_screen();
  void refresh_screen(int n);
  void show_error_msg(char *s);

  char curr_screen;
private:
  // repo:
  int r_f, r_e, r_c, r_free_space;
  // char *r_time;
  // bool r_gps_valid;
  int lora_wheel;
  char *wheel;
};

#endif // _INCLUDE_UI_TBEAM_H
