// ui-watch.h

#if defined(TINYSSB_BOARD_TWATCH)

#ifndef _INCLUDE_UI_TWATCH_H
#define _INCLUDE_UI_TWATCH_H

#include "lib/tinySSBlib.h"
#include <lvgl.h>

class UI_TWatch_Class: public UIClass {

  using UIClass::UIClass;

public:
  UI_TWatch_Class();

  void refresh();
  
  void loop() override; // for screen animations
  void spinner(bool show) override;
  void boot_ended() override;

  lv_obj_t *scr;
  lv_style_t bg_style;
  lv_obj_t *spin;

  // void buzz() override;
  void to_next_screen();

  int post_cnt = 0;
  void add_new_post(char *txt, int t, int pos) override;

  signing_fct my_signing_fct;
  bool myid_valid;
  unsigned char myid_pk[ED25519_PK_LEN];
  unsigned char myid_sk[ED25519_SK_LEN];
};

#endif // _INCLUDE_UI_TWATCH_H
#endif // TINYSSB_BOARD_TWATCH
