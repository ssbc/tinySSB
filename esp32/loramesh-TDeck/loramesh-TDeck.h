// loramesh_TDeck.h


#define LV_USE_PERF_MONITOR 0 // unfortunately this is not honored, had to patch cpp

#include "config.h"  // all definitions and header files for TDeck

// #define MyFS LittleFS

#include "hw_setup.h"
#include "ui_setup.h"


void setup()
{
  hw_setup();
  ui_setup();
}


void loop()
{
    button.check();
    // loopRadio();
    lv_task_handler();
    delay(5);
}

// eof
