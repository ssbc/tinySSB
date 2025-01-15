// hardware.cpp

// collects hardware-specific init code (other than UI) for all boards

#include "lib/tinySSBlib.h"
#include "hardware.h"

// ---------------------------------------------------------------------------
#if defined(TINYSSB_BOARD_HELTEC) || defined(TINYSSB_BOARD_HELTEC3)

#ifdef HAS_LORA

#ifdef USING_SX1262
  SX1262 radio = new Module(SS, DIO1, RST_LoRa, BUSY_LoRa);
#endif
#ifdef USING_SX1276
  SX1276 radio = new Module(SS, DI0, RST);
#endif
#ifdef USING_SX1278
  SX1278 radio = new Module(SS, DI0, RST);
#endif

#endif // HAS_LORA


void hw_init()
{
#ifdef HAS_LORA
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
#endif // HAS_LORA
}

#endif // HELTEC, HELTEC3

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
  SX1262 sx1262_radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                                   RADIO_RST_PIN, RADIO_BUSY_PIN);
# endif
#endif

void hw_init()
{

  extern TFT_eSPI        tft;
  extern SemaphoreHandle_t xSemaphore;

  //! The board peripheral power control pin needs to
  //  be set to HIGH when using the peripheral
  pinMode(BOARD_POWERON, OUTPUT);
  digitalWrite(BOARD_POWERON, HIGH);

  //! Set CS on all SPI buses to high level during initialization
  pinMode(BOARD_SDCARD_CS, OUTPUT);
  pinMode(RADIO_CS_PIN, OUTPUT);
  pinMode(BOARD_TFT_CS, OUTPUT);

  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  pinMode(BOARD_SPI_MISO, INPUT_PULLUP);
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

  pinMode(BOARD_BOOT_PIN, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G02, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G01, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G04, INPUT_PULLUP);
  pinMode(BOARD_TBOX_G03, INPUT_PULLUP);

  //Wakeup touch chip
  pinMode(BOARD_TOUCH_INT, OUTPUT);
  digitalWrite(BOARD_TOUCH_INT, HIGH);

  //Add mutex to allow multitasking access
  xSemaphore = xSemaphoreCreateBinary();
  assert(xSemaphore);
  xSemaphoreGive( xSemaphore );

  /*
  delay(1000);
  Serial.println("a");
  tft.begin(0);
  Serial.println("b");
  tft.setRotation( 1 );
  tft.fillScreen(TFT_BLACK);
  */

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  // Set touch int input
  pinMode(BOARD_TOUCH_INT, INPUT); delay(20);

  // digitalWrite(BOARD_SDCARD_CS, HIGH);
  // digitalWrite(RADIO_CS_PIN, HIGH);
  // digitalWrite(BOARD_TFT_CS, HIGH);

  // SPI.end();
  // SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("RadioLib sx1262 init success!");
  } else {
    Serial.print(F("RadioLib begin() for sx1262 failed, code "));
    Serial.println(state);
    delay(2000);
  }
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TWATCH

#include "XPowersLib.h"
XPowersPMU power;

#ifdef USE_RADIO_LIB
  SPIClass radioBus = SPIClass(HSPI);
  // SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
# ifdef USING_SX1262
  SX1262 radio = new Module(BOARD_RADIO_SS, BOARD_RADIO_DI01,
                            BOARD_RADIO_RST, BOARD_RADIO_BUSY, radioBus);
# endif
#endif // USE_RADIO_LIB

void hw_init()
{
  setCpuFrequencyMhz(240);

  power.init(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL);
  power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
  power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_900MA);
  power.setSysPowerDownVoltage(2600);
  //! Radio VDD , Don't change
  power.setALDO4Voltage(3300);
  power.enableALDO4();  //! Radio VDD

  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  SPI.end();
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD
  // radioBus.begin(BOARD_RADIO_SCK, BOARD_RADIO_MISO, BOARD_RADIO_MOSI); 

  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("RadioLib sx1262 init success!");
  } else {
    Serial.printf("RadioLib sx1262 init did not work (%d)\r\n", state);
    delay(2000);
  }
  */
}

#endif // TINYSSB_BOARD_TWATCH

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_TWRIST

void hw_init()
{
}

#endif

// ---------------------------------------------------------------------------
#ifdef TINYSSB_BOARD_WLPAPER

#ifdef HAS_LORA
  SPIClass spi(SPI);
  SPISettings spiSettings(2000000, MSBFIRST, SPI_MODE0);
# ifdef USING_SX1262
  SX1262 radio = new Module(RADIO_NSS, RADIO_DIO_1, RADIO_RESET, RADIO_BUSY, spi); // , spiSettings);
#endif

#endif // HAS_LORA

void hw_init()
{
  setCpuFrequencyMhz(240);

  /*
  // is this necessary, or helpful?
  pinMode(VEXT, INPUT); // OFF
  delay(50);
  pinMode(VEXT, OUTPUT); // ON
  digitalWrite(VEXT, LOW);
  delay(100);
  */

#ifdef HAS_LORA
  // begin(float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10, uint16_t preambleLength = 8, float tcxoVoltage = 1.6, bool useRegulatorLDO = false);
  spi.begin(LORA_CLK, LORA_MISO, LORA_MOSI, RADIO_NSS); //SCK, MISO, MOSI, SS
  int state = radio.begin(902.4, 125, 7);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("RadioLib success!");
  } else {
    while (true) {
      Serial.print(F("RadioLib failed, code "));
      Serial.println(state);
      delay(2000);
    }
  }
#endif // HAS_LORA
}

#endif // WIRELESS_PAPER

// ---------------------------------------------------------------------------

// eof
