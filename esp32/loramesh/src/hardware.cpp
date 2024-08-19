// hardware.cpp

// collects hardware-specific init code (other than UI) for all boards

#include "lib/tinySSBlib.h"
#include "hardware.h"

// ---------------------------------------------------------------------------
#if defined(TINYSSB_BOARD_HELTEC) || defined(TINYSSB_BOARD_HELTEC3)

#ifdef USING_SX1262
  SX1262 radio = new Module(SS, DI0, RST);
#endif
#ifdef USING_SX1276
  SX1276 radio = new Module(SS, DI0, RST);
#endif
#ifdef USING_SX1278
  SX1278 radio = new Module(SS, DI0, RST);
#endif

void hw_init()
{
  // SX1278 has the following connections:
  // NSS pin:   10
  // DIO0 pin:  2
  // NRST pin:  9
  // DIO1 pin:  3
  // = new Module(10, 2, 9, 3);

  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("RadioLib success!");
  } else {
    while (true) {
      Serial.print(F("RadioLib failed, code "));
      Serial.println(state);
      delay(2000);
    }
  }  
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

#ifdef USE_RADIO_LIB
# ifdef USING_SX1262
    SX1262 radio_sx1262 = new Module(SS, DI0, RST); // , RADIO_BUSY_PIN);
# endif
# ifdef USING_SX1276
    SX1276 radio_sx1276 = new Module(SS, DI0, RST); // , RADIO_BUSY_PIN);
# endif

RadioChoice fused_radio;

#endif

void hw_init()
{
  Wire.begin(21, 22);

  SPI.end();
  SPI.begin(SCK, MISO, MOSI); // copied from T-Deck code
  
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

  int state = radio_sx1262.begin();
  if (state == RADIOLIB_ERR_NONE) {
    fused_radio.is_62 = true;
    Serial.println("RadioLib sx1262 init success!");
  } else {
    Serial.printf("RadioLib sx1262 init did not work (code %d), trying sx1276\r\n", state);
    state = radio_sx1276.begin();
    if (state == RADIOLIB_ERR_NONE) {
      fused_radio.is_62 = false;
      Serial.println("RadioLib sx1276 init success!");
    } else while (true) {
      Serial.print(F("RadioLib for both sx1276 and sx1262 failed, code "));
      Serial.println(state);
      delay(2000);
    }
  }
  
}

#endif // _TBEAM

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TDECK

#ifdef USE_RADIO_LIB
# ifdef USING_SX1262
    SX1262 radio_sx1262 = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                                     RADIO_RST_PIN, RADIO_BUSY_PIN); // , RADIO_BUSY_PIN);
# endif
# ifdef USING_SX1276
    SX1276 radio_sx1276 = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                                     RADIO_RST_PIN, RADIO_BUSY_PIN); // , RADIO_BUSY_PIN);
# endif

RadioChoice fused_radio;

#endif

void hw_init()
{
  SPI.end();
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

  int state = radio_sx1262.begin();
  if (state == RADIOLIB_ERR_NONE) {
    fused_radio.is_62 = true;
    Serial.println("RadioLib sx1262 init success!");
  } else {
    Serial.printf("RadioLib sx1262 init did not work (%d), trying sx1276\r\n", state);
    state = radio_sx1276.begin();
    if (state == RADIOLIB_ERR_NONE) {
      fused_radio.is_62 = false;
      Serial.println("RadioLib sx1276 init success!");
    } else while (true) {
      Serial.print(F("RadioLib for both sx1276 and sx1262 failed, code "));
      Serial.println(state);
      delay(2000);
    }
  }
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TWRIST

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------

// eof
