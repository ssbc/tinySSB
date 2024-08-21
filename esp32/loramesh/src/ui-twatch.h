// ui-watch.h

#if defined(TINYSSB_BOARD_TWATCH)

#ifndef _INCLUDE_UI_TWATCH_H
#define _INCLUDE_UI_TWATCH_H

#include "lib/tinySSBlib.h"
// #include <lvgl.h>

class UI_TWatch_Class: public UIClass {

  using UIClass::UIClass;

public:
  UI_TWatch_Class();

  void to_next_screen();
  void refresh();
  
#if 0
  void spinner(bool show) override;
  void buzz() override;
  void loop() override; // for screen animations


  void _brightness_cb(lv_event_t *e);
  void _four_button_cb(lv_event_t *e);
private:
  lv_obj_t *scr;
  lv_style_t bg_style;
  
  lv_obj_t *btn_left;
  lv_obj_t *bar;
  lv_obj_t *btn_right;
  lv_obj_t *posts; // flex layout
  lv_obj_t *log_lbl;
  lv_obj_t *four_buttons[4], *current_btn;
  lv_obj_t *spin;

  lv_obj_t *hz[3];
#endif // 0
};

#endif // _INCLUDE_UI_TWATCH_H
#endif // TINYSSB_BOARD_TWATCH
