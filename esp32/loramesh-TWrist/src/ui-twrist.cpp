// ui-twrist.cpp

#if defined(TINYSSB_BOARD_TWRIST)

#include "lib/tinySSBlib.h"
#include "epd-scuttleshell.h"

#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
// #include <GxGDEH0154Z90/GxGDEH0154Z90.h>  // 1.54" b/w/r 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#define PIN_MOTOR 4
#define PIN_KEY GPIO_NUM_35
#define PWR_EN 5
#define Backlight 33

#define SPI_SCK 14
#define SPI_DIN 13
#define EPD_CS 15
#define EPD_DC 2
#define SRAM_CS -1
#define EPD_RESET 17
#define EPD_BUSY 16

#include <Fonts/FreeMonoBold9pt7b.h>

static GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
static GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);


UIClass::UIClass()
{
  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PWR_EN, OUTPUT);

  gXdisplay.init();
  gXdisplay.setRotation(0);
  gXdisplay.fillScreen(GxEPD_BLACK);
  gXdisplay.drawBitmap(0, 0, scuttleshell, 200, 200, GxEPD_WHITE);
  gXdisplay.update();

}


void UIClass::to_next_screen()
{
  gXdisplay.setFont(&FreeMonoBold9pt7b);
  gXdisplay.setTextColor(GxEPD_WHITE);
  gXdisplay.setCursor(60, 40);
  gXdisplay.println("hello1");
  gXdisplay.setTextColor(GxEPD_BLACK);
  gXdisplay.setCursor(60, 60);
  gXdisplay.println("hello2");
  gXdisplay.update();
}


void UIClass::loop() {}


void UIClass::buzz()
{
  digitalWrite(PWR_EN, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
  delay(100);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
}


/*
void DeepSleep(void)
{
  Serial.println("switching to deep sleep mode");

  gXdisplay.powerDown();
  pinMode(PIN_MOTOR, INPUT_PULLDOWN);
  pinMode(PWR_EN, INPUT);
  pinMode(PIN_KEY, INPUT);
  pinMode(SPI_SCK, INPUT);
  pinMode(SPI_DIN, INPUT);
  pinMode(EPD_CS, INPUT);
  pinMode(EPD_DC, INPUT);
  pinMode(EPD_RESET, INPUT);
  pinMode(EPD_BUSY, INPUT);
  pinMode(PIN_MOTOR, INPUT);
  pinMode(Backlight, INPUT),
  esp_sleep_enable_ext0_wakeup(PIN_KEY, 0);
  esp_deep_sleep_start();
}
*/

#endif // TINYSSB_BOARD_TWRIST

// eof
