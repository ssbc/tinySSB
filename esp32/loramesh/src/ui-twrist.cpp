// ui-twrist.cpp

#if defined(TINYSSB_BOARD_TWRIST)

#include <cstdarg>   // for va_list

#include "lib/tinySSBlib.h"
#include "ui-twrist.h"

#include "const-twrist.h"

void cmd_rx(String s) {}

// ---------------------------------------------------------------------------

#include "lib/inflate.h"
#include "ui-twrist/epd-scuttleshell_z.h"
#include "ui-twrist/epd-crayon_z.h"
#include "ui-twrist/epd-hieroglyph_z.h"
#include "ui-twrist/epd-bird_z.h"
#include "ui-twrist/epd-oakland_z.h"
#include "ui-twrist/epd-map_z.h"

int pict = 0; // cycle through the pics
#define CYCLE_TIME (2*60*1000) // once every 2 minutes
long next_cycle;

#define BITMAP_SIZE (200*200/8)
unsigned char scuttleshell_bw[BITMAP_SIZE];
unsigned char crayon_bw[BITMAP_SIZE];
unsigned char hieroglyph_bw[BITMAP_SIZE];
unsigned char bird_bw[BITMAP_SIZE];
unsigned char oakland_bw[BITMAP_SIZE];
unsigned char map_bw[BITMAP_SIZE];

unsigned char* gallery[] = {
  scuttleshell_bw, crayon_bw, hieroglyph_bw,
  bird_bw, oakland_bw, map_bw
};

// ---------------------------------------------------------------------------

#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
// #include <GxGDEH0154Z90/GxGDEH0154Z90.h>  // 1.54" b/w/r 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Fonts/FreeMonoBold9pt7b.h>

static GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
static GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);

// ---------------------------------------------------------------------------

#include <Button2.h>

Button2 userButton;

void clicked(Button2& btn) {
  Serial.println("click");
  ((UI_TWrist_Class*)theUI)->refresh();
}

void long_clicked(Button2& btn) {
  Serial.println("long_click");
  pict = 0;
  ((UI_TWrist_Class*)theUI)->to_next_screen();
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
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PWR_EN, OUTPUT);

  pinMode(PIN_KEY, INPUT_PULLUP);
  userButton.begin(PIN_KEY);
  userButton.setLongClickTime(1000);
  userButton.setClickHandler(clicked);
  userButton.setLongClickDetectedHandler(long_clicked);
}


UI_TWrist_Class::UI_TWrist_Class()
{
  wifi_cnt = 0;

  // show_lora_specs("USA 902a", 902500000, 250000, 7);

#define INFLATE(NM) inflate(NM##_bw, sizeof(NM##_bw),   \
                            NM##_z, sizeof(NM##_z));
  INFLATE(scuttleshell)
  INFLATE(crayon)
  INFLATE(hieroglyph)
  INFLATE(bird)
  INFLATE(oakland)
  INFLATE(map)
                                       
  gXdisplay.init();
  gXdisplay.setRotation(0);
  gXdisplay.setFont(&FreeMonoBold9pt7b);
  gXdisplay.setTextColor(GxEPD_BLACK);

  gXdisplay.drawBitmap(scuttleshell_bw, sizeof(scuttleshell_bw),
                       GxEPD::bm_partial_update);
  
  // slowly appearing logo:
  // gXdisplay.drawBitmap(scuttleshell_bw, 200*200/8, GxEPD::bm_partial_update);
  // gXdisplay.drawBitmap(scuttleshell_bw, 200*200/8, GxEPD::bm_partial_update);

  curr_screen = SCREEN_SPLASH;
  next_cycle = millis() + CYCLE_TIME;
}


void UI_TWrist_Class::buzz()
{
  digitalWrite(PWR_EN, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(100);
  digitalWrite(PIN_MOTOR, LOW);
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


void UI_TWrist_Class::loop()
{
  userButton.loop();

  if (curr_screen == SCREEN_SPLASH && next_cycle !=0 && next_cycle < millis())
    refresh_screen(curr_screen);
}


void UI_TWrist_Class::refresh()
{
  refresh_screen(curr_screen);
}


void UI_TWrist_Class::to_next_screen()
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

  gXdisplay.setCursor(0, y);
  gXdisplay.println(k);

  gXdisplay.setCursor(55, y);
  gXdisplay.println(buf);
}

void UI_TWrist_Class::refresh_screen(int scr)
{
  if (scr != curr_screen)
      return;

  if (scr == SCREEN_SPLASH) {
    gXdisplay.drawBitmap(gallery[pict], 0, 0, 200, 200, GxEPD_WHITE);
    gXdisplay.setCursor(145, 11);
    gXdisplay.println(ssid+7);
    pict = (pict + 1) % (sizeof(gallery) / sizeof(unsigned char*));
    next_cycle = millis() + CYCLE_TIME;
/*
  } else if (false && scr == SCREEN_NODE) {
    gXdisplay.fillScreen(GxEPD_WHITE);
    gXdisplay.setCursor(5, 15);
    gXdisplay.println("node");

    // gXdisplay.setCursor(0, 200);
    // gXdisplay.println(">");

    gXdisplay.setCursor(75, 202);
    gXdisplay.println("o - -");
*/
  } else if (scr == UI_TWrist_Class::SCREEN_REPO) {
    gXdisplay.fillScreen(GxEPD_WHITE);

    gXdisplay.setCursor(0, 11);
    gXdisplay.println("Node");
    gXdisplay.setCursor(145, 11);
    gXdisplay.println(ssid+7);
    for (int i = 0; i < 200; i++)
      gXdisplay.drawPixel(i, 16, GxEPD_BLACK);

    /*
    pos_key_val(45, "LoRa", "%.2f/%d/%d",
                lora_fr/1000000.0, lora_bw/1000, lora_sf);
    */

    pos_key_val(45, "Repo", "%dF %dE %dC", f_cnt, e_cnt, c_cnt);
    
    int fb = MyFS.totalBytes() -  MyFS.usedBytes();
    pos_key_val(70, "Free", "%.1fMB (%d%%)",
                fb / 1024.0 / 1024, fb * 100 / MyFS.totalBytes());

    pos_key_val(95, "BLE", "%d", ble_cnt);
    
    pos_key_val(120, "WiFi", "%d", wifi_cnt);

    gXdisplay.setCursor(85, 202);
    gXdisplay.println("o -");
  } else if (scr == UI_TWrist_Class::SCREEN_PEERS) {
    gXdisplay.fillScreen(GxEPD_WHITE);

    gXdisplay.setCursor(5, 11);
    gXdisplay.println("Peers");
    gXdisplay.setCursor(145, 11);
    gXdisplay.println(ssid+7);
    for (int i = 0; i < 200; i++)
      gXdisplay.drawPixel(i, 16, GxEPD_BLACK);

    gXdisplay.setCursor(85, 202);
    gXdisplay.println("- o");
  }

  gXdisplay.update();

  /*
  for (int y = 190; y < 200; y++)
    for (int x = 0; x < 10; x++)
      gXdisplay.drawPixel(x, y, GxEPD_BLACK);
  gXdisplay.updateToWindow(0, 190, 0, 190, 10, 10);
  */
}


/*
void printPower(uint16_t x, uint16_t y)
{
  pinMode(Bat_ADC, ANALOG);
  adcAttachPin(Bat_ADC);
  analogReadResolution(12);
  //display.fillRect(x - 32, y, 28, 12, GxEPD_WHITE); //clean
  display.drawRect(x, y, 28, 12, GxEPD_BLACK);
  display.drawRect(x + 28, y + 3, 3, 6, GxEPD_BLACK);
  int bat = 0;
  for (uint8_t i = 0; i < 20; i++)
  {
    bat += analogRead(Bat_ADC);
  }
  bat /= 20;
  float volt = bat * 3.3 / 4096;
  display.fillRect(x + 2, y + 2, 24 * (bat > 2700 ? 1 : volt / 4.2), 8, GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
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
