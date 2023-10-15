// hardware.cpp

// collects hardware-specific init code (other than UI) for all boards

#include "lib/tinySSBlib.h"

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_HELTEC

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_T5GRAY

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TBEAM

#include <axp20x.h>
#include <Wire.h>
AXP20X_Class axp;

#ifdef HAS_GPS
TinyGPSPlus gps;
HardwareSerial GPS(1);
#endif

void hw_init()
{
  Wire.begin(21, 22);
  if (axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin FAIL");
  } else {
    // Serial.println("AXP192 Begin PASS");
#ifdef HAS_LORA
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
#else
    axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
#endif
#ifdef HAS_GPS
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
#else
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
#endif
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
#ifdef HAS_OLED
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED
#else
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF); // no OLED
#endif

#ifdef HAS_OLED
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high、
#endif
    
#ifdef HAS_GPS
    GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
#endif
  }
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TDECK

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TWRIST

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_WIFIPAPER

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
// eof
