// ui.h

#ifndef _INCLUDE_UI_H
#define _INCLUDE_UI_H

#include "config.h"


class UIClass {
  
public:
  UIClass();
  void tft_onOff(bool lit); // 0=off
  void refresh();
  void spinner(bool show);
  void brightness_cb(lv_event_t *e);
  void four_button_cb(lv_event_t *e);

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
};


#endif
