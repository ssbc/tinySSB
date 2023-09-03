// ui.h

#ifndef _INCLUDE_UI_H
#define _INCLUDE_UI_H

class UIClass {
  
public:
  UIClass();
  virtual ~UIClass();
  virtual void spinner(bool show) {};
  virtual void to_next_screen() {};
  virtual void buzz() {};
  virtual void loop() {}; // for screen animations

private:
};


#endif
