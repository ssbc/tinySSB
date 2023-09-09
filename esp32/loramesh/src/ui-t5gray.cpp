// ui-t5gray.cpp

#if defined(TINYSSB_BOARD_T5GRAY)

#include <cstdarg>   // for va_list

#include "lib/tinySSBlib.h"
#include "ui-t5gray.h"

#include "const-t5gray.h"

void cmd_rx(String s) {}

// ---------------------------------------------------------------------------

#include "lib/inflate.h"
#include "ui-t5gray/scuttleshell_z.h"
#include "ui-t5gray/mushroom_z.h"
/*
#include "ui-twrist/epd-crayon_z.h"
#include "ui-twrist/epd-hieroglyph_z.h"
#include "ui-twrist/epd-bird_z.h"
#include "ui-twrist/epd-oakland_z.h"
#include "ui-twrist/epd-map_z.h"
*/
int pict = 0; // cycle through the pics
// #define CYCLE_TIME (2*60*1000) // once every 2 minutes
#define CYCLE_TIME (20*1000) // 20 sec
long next_cycle;

#define BITMAP_SIZE (212*104/8)
unsigned char scuttleshell_bw[BITMAP_SIZE];
unsigned char scuttleshell_gr[BITMAP_SIZE];
unsigned char mushroom_bw[BITMAP_SIZE];
unsigned char mushroom_gr[BITMAP_SIZE];
/*
unsigned char crayon_bw[BITMAP_SIZE];
unsigned char hieroglyph_bw[BITMAP_SIZE];
unsigned char bird_bw[BITMAP_SIZE];
unsigned char oakland_bw[BITMAP_SIZE];
unsigned char map_bw[BITMAP_SIZE];
*/
unsigned char* gallery_bw[] = {
  scuttleshell_bw, mushroom_bw
};
unsigned char* gallery_gr[] = {
  scuttleshell_gr, mushroom_gr
};

// ---------------------------------------------------------------------------

/*
#include <GxEPD.h>
// #include <GxGDEW0213M21/GxGDEW0213M21.h>
#include <GxGDEW0213T5/GxGDEW0213T5.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Fonts/FreeMonoBold9pt7b.h>
*/

// static GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
// static GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);

#include "ui-t5gray/ePaperDriverLib/ePaperDriver.h"
// #include <Fonts/Picopixel.h>
// #include <Fonts/Org_01.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
// #include <Fonts/FreeSans9pt7b.h>
// #include <Fonts/FreeSansBold18pt7b.h>

// ePaperDisplay *device = new ePaperDisplay( GDEW0213T5, 21, 2, 4, 5 );
ePaperDisplay *device;
#define gXdisplay (*device)

// ---------------------------------------------------------------------------

#include <Button2.h>

Button2 userButton;

void clicked(Button2& btn) {
  Serial.println("click");
  ((UI_T5gray_Class*)theUI)->refresh();
}

void long_clicked(Button2& btn) {
  Serial.println("long_click");
  pict = 0;
  ((UI_T5gray_Class*)theUI)->to_next_screen();
}

// ---------------------------------------------------------------------------

UIClass::UIClass()
{
  node_name = time = lora_profile = NULL;
  gps_lon = gps_lat = gps_ele = 0.0;
  f_cnt = e_cnt = c_cnt = 0;
  ble_cnt = wifi_cnt = 0;
  lora_fr = lora_bw = lora_sf = 0;
  
  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  device = new ePaperDisplay(GDEW0213T5,
                             EPD_BUSY, EPD_RESET, EPD_DC, EPD_CS);
  Serial.printf("UI_T5gray_Class init %p\r\n", device);

  // pinMode(PIN_MOTOR, OUTPUT);
  // pinMode(PWR_EN, OUTPUT);

  pinMode(PIN_KEY, INPUT_PULLUP);
  userButton.begin(PIN_KEY);
  userButton.setLongClickTime(1000);
  userButton.setClickHandler(clicked);
  userButton.setLongClickDetectedHandler(long_clicked);
}


UI_T5gray_Class::UI_T5gray_Class()
{
  wifi_cnt = 0;

  // show_lora_specs("USA 902a", 902500000, 250000, 7);

#define INFLATE(NM) inflate(NM##_bw, sizeof(NM##_bw),      \
                            NM##_bw_z, sizeof(NM##_bw_z)); \
                    inflate(NM##_gr, sizeof(NM##_gr),      \
                            NM##_gr_z, sizeof(NM##_gr_z));
  INFLATE(scuttleshell)
  INFLATE(mushroom)
/*    
  INFLATE(crayon)
  INFLATE(hieroglyph)
  INFLATE(bird)
  INFLATE(oakland)
  INFLATE(map)
*/                                       
  gXdisplay.setRotation(1);
  // gXdisplay.setFont(&FreeMonoBold9pt7b);
  // gXdisplay.setFont(&FreeMono9pt7b);

  gXdisplay.fillScreen(ePaper_WHITE);
  gXdisplay.setTextColor(ePaper_BLACK);
  gXdisplay.setCursor(20, 50);
  gXdisplay.print("starting");
  // gXdisplay.fillCircle(70, 70, 30, ePaper_BLACK);

  /*
  gXdisplay.drawBitImage(0, 0, 212, 104,
                         gallery_bw[pict], 212*104/8, false,
                         gallery_gr[pict], 212*104/8, false);

  */
  gXdisplay.refreshDisplay();

  curr_screen = SCREEN_SPLASH;
  next_cycle = millis() + CYCLE_TIME;
}


void UI_T5gray_Class::buzz()
{
  /*  digitalWrite(PWR_EN, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(100);
  digitalWrite(PIN_MOTOR, LOW);
  */
  /*
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
  delay(100);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
  */
}


void UI_T5gray_Class::loop()
{
  userButton.loop();

  if (curr_screen == SCREEN_SPLASH && next_cycle !=0 && next_cycle < millis()) {
    Serial.println("next image");
    refresh_screen(curr_screen);
  }
}


void UI_T5gray_Class::refresh()
{
  refresh_screen(curr_screen);
}


void UI_T5gray_Class::to_next_screen()
{
  curr_screen = (curr_screen + 1) % SCREEN_OFF;
  refresh_screen(curr_screen);
}


void pos_key_val(int y, char *k, char *fmt, ...)
{
  va_list args;
  char buf[50];

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  gXdisplay.setCursor(3, y);
  gXdisplay.print(k);

  gXdisplay.setCursor(55, y);
  gXdisplay.print(buf);
}

void UI_T5gray_Class::refresh_screen(int scr)
{
  if (scr != curr_screen)
      return;

  if (scr == SCREEN_SPLASH) {
    gXdisplay.fillScreen(ePaper_GRAY2);
    // gXdisplay.drawBitmap(gallery[pict], 0, 0, 200, 200, ePaper_WHITE);
    // gXdisplay.setDeviceImage(gallery[pict], 200*200/8, false);
    gXdisplay.drawBitImage(0, 0, 212, 104,
                           gallery_bw[pict], 212*104/8, false,
                           gallery_gr[pict], 212*104/8, false);

    gXdisplay.setFont(&FreeMonoBold9pt7b);
    // gXdisplay.setCursor(3, 102);
    // gXdisplay.print(String(pict));
    
    gXdisplay.setCursor(153, 13);
    gXdisplay.print(ssid+7);
    pict = (pict + 1) % (sizeof(gallery_bw) / sizeof(unsigned char*));
    next_cycle = millis() + CYCLE_TIME;
  } else if (scr == UI_T5gray_Class::SCREEN_REPO) {
    gXdisplay.fillScreen(ePaper_WHITE);

    gXdisplay.setFont(&FreeMonoBold9pt7b);
    gXdisplay.setCursor(3, 13);
    gXdisplay.print("Node");
    gXdisplay.setCursor(153, 13);
    gXdisplay.print(ssid+7);
    for (int i = 0; i < 212; i++)
      gXdisplay.drawPixel(i, 17, ePaper_BLACK);
    gXdisplay.setCursor(95, 106);
    gXdisplay.print("o -");

    gXdisplay.setFont(&FreeMono9pt7b);
    // gXdisplay.setFont(&Org_01);
    pos_key_val(35, "Repo", "%dF %dE %dC", f_cnt, e_cnt, c_cnt);
    
    int fb = MyFS.totalBytes() -  MyFS.usedBytes();
    pos_key_val(52, "Free", "%.1fMB (%d%%)",
                fb / 1024.0 / 1024, fb * 100 / MyFS.totalBytes());

    pos_key_val(69, "BLE", "%d", ble_cnt);
    
    pos_key_val(86, "WiFi", "%d", wifi_cnt);

  } else if (scr == UI_T5gray_Class::SCREEN_PEERS) {
    gXdisplay.fillScreen(ePaper_WHITE);

    gXdisplay.setFont(&FreeMonoBold9pt7b);
    gXdisplay.setCursor(3, 13);
    gXdisplay.println("Peers");
    gXdisplay.setCursor(153, 13);
    gXdisplay.println(ssid+7);
    for (int i = 0; i < 212; i++)
      gXdisplay.drawPixel(i, 17, ePaper_BLACK);
    gXdisplay.setCursor(95, 106);
    gXdisplay.print("- o");
  }

  // gXdisplay.update();
  gXdisplay.refreshDisplay();

  /*
  for (int y = 190; y < 200; y++)
    for (int x = 0; x < 10; x++)
      gXdisplay.drawPixel(x, y, ePaper_BLACK);
  gXdisplay.updateToWindow(0, 190, 0, 190, 10, 10);
  */
}


/*
void printPower(uint16_t x, uint16_t y)
{
  pinMode(Bat_ADC, ANALOG);
  adcAttachPin(Bat_ADC);
  analogReadResolution(12);
  //display.fillRect(x - 32, y, 28, 12, ePaper_WHITE); //clean
  display.drawRect(x, y, 28, 12, ePaper_BLACK);
  display.drawRect(x + 28, y + 3, 3, 6, ePaper_BLACK);
  int bat = 0;
  for (uint8_t i = 0; i < 20; i++)
  {
    bat += analogRead(Bat_ADC);
  }
  bat /= 20;
  float volt = bat * 3.3 / 4096;
  display.fillRect(x + 2, y + 2, 24 * (bat > 2700 ? 1 : volt / 4.2), 8, ePaper_BLACK);
  display.setTextColor(ePaper_BLACK);
  display.setTextSize(1);
  display.setCursor(x - 32, y + 4);
  display.print(volt);
  display.print("V");
}
*/

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
