// ui-tdeck.h

#ifndef _INCLUDE_UI_TDECK_H
#define _INCLUDE_UI_TDECK_H

#include "lib/tinySSBlib.h"
#include <lvgl.h>

class UI_TDeck_Class: public UIClass {

  using UIClass::UIClass;

public:
  UI_TDeck_Class();
  void spinner(bool show) override;
  void to_next_screen() override;
  void buzz() override;
  void loop() override; // for screen animations

  void refresh();

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
};

#endif // _INCLUDE_UI_TDECK_H
