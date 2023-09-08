// ui-twrist.h

#ifndef _INCLUDE_UI_TWRIST_H
#define _INCLUDE_UI_TWRIST_H

#include "lib/tinySSBlib.h"

class UI_TWrist_Class: public UIClass {

  using UIClass::UIClass;

  enum {
    SCREEN_SPLASH,
    // SCREEN_NODE,
    SCREEN_REPO,
    SCREEN_PEERS,
    SCREEN_OFF
  };

public:
  UI_TWrist_Class();
  void buzz() override;
  void loop() override; // for screen animations
  void refresh() override;

  void to_next_screen();
  void refresh_screen(int n);

private:
  char curr_screen;
};

#endif // _INCLUDE_UI_TWRIST_H
