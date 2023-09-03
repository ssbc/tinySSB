// ui-twrist.cpp

#if defined(TINYSSB_BOARD_TWRIST)

#include "lib/tinySSBlib.h"
#include "ui-twrist.h"
#include "ui-twrist/epd-scuttleshell.h"

#include "const-twrist.h"

// ---------------------------------------------------------------------------

#include <GxEPD.h>
#include <GxDEPG0150BN/GxDEPG0150BN.h>    // 1.54" b/w 200x200
// #include <GxGDEH0154Z90/GxGDEH0154Z90.h>  // 1.54" b/w/r 200x200
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include <Fonts/FreeMono9pt7b.h>

static GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
static GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);

// ---------------------------------------------------------------------------

#include <Button2.h>

Button2 userButton;
bool lora_selection; // true if lora plan selection ongoing, else do_io
long selection_timeout;

void clicked(Button2& btn) {
  Serial.println("click");
  selection_timeout = millis() + 10000;

  /*
  if (lora_selection) { // cycle through the choices
    int i = the_lora_config - lora_configs;
    i = (i+1) % lora_configs_size;
    the_lora_config = lora_configs + i;
  } else
  */
  ((UI_TWrist_Class*)theUI)->to_next_screen();
}

void long_clicked(Button2& btn) {
  Serial.println("long_click");
  return;
  selection_timeout = millis() + 10000;

  if (lora_selection) { // this is our choice
    /*
    struct bipf_s *v = bipf_mkString(the_lora_config->plan);
    bipf_dict_set(the_config, bipf_mkString("lora_plan"), v);
    config_save(the_config);

    char s[30];
    theDisplay.clear();
    theDisplay.setFont(ArialMT_Plain_24);
    sprintf(s, ">> %s", the_lora_config->plan);
    theDisplay.drawString(0, 10, s);
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0, 46, "rebooting now");
    theDisplay.display();

    delay(3000);
    lora_log.printf(">> reboot cmd\n");
    lora_log.close();
    esp_restart();
    */
  } else
    lora_selection = !lora_selection;
}

// ---------------------------------------------------------------------------

UIClass::UIClass()
{
  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PWR_EN, OUTPUT);

  pinMode(PIN_KEY, INPUT_PULLUP);
  userButton.begin(PIN_KEY);
  userButton.setLongClickTime(2000);
  userButton.setClickHandler(clicked);
  userButton.setLongClickDetectedHandler(long_clicked);
}


UI_TWrist_Class::UI_TWrist_Class()
{
  gXdisplay.init();
  gXdisplay.setRotation(0);
  gXdisplay.setFont(&FreeMono9pt7b);
  gXdisplay.setTextColor(GxEPD_BLACK);

  gXdisplay.drawBitmap(scuttleshell, 200*200/8, GxEPD::bm_partial_update);
  // slowly appearing logo:
  // gXdisplay.drawBitmap(scuttleshell, 200*200/8, GxEPD::bm_partial_update);
  // gXdisplay.drawBitmap(scuttleshell, 200*200/8, GxEPD::bm_partial_update);

  curr_screen = SCREEN_SPLASH;
}


void UI_TWrist_Class::buzz()
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


void UI_TWrist_Class::loop()
{
  userButton.loop();
}


void UI_TWrist_Class::refresh()
{
  refresh_screen(curr_screen);
}


void UI_TWrist_Class::to_next_screen()
{
  curr_screen = (curr_screen + 1) % SCREEN_OFF;
  refresh_screen(curr_screen);
  /*
  gXdisplay.setFont(&FreeMonoBold9pt7b);
  gXdisplay.setTextColor(GxEPD_WHITE);
  gXdisplay.setCursor(60, 40);
  gXdisplay.println("hello1");
  gXdisplay.setTextColor(GxEPD_BLACK);
  gXdisplay.setCursor(60, 60);
  gXdisplay.println("hello2");
  gXdisplay.update();
  */
}


void UI_TWrist_Class::refresh_screen(int scr)
{
  if (scr != curr_screen)
      return;

  if (scr == SCREEN_SPLASH) {
    gXdisplay.drawBitmap(scuttleshell, 0, 0, 200, 200, GxEPD_WHITE);
  } else if (scr == SCREEN_NODE) {
    gXdisplay.fillScreen(GxEPD_WHITE);
    gXdisplay.setCursor(60, 40);
    gXdisplay.println("node");
    /*
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0 , 0, "SSB.virt.lora.pub");
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0 , 17, __DATE__ " " __TIME__ UTC_OFFSET);

    int f = the_lora_config->fr / 10000;
    char fr[30];
    sprintf(fr, "%d.%02d MHz", f/100, f%100);
    theDisplay.setFont(ArialMT_Plain_24);
    theDisplay.drawString(0, 30, fr);
    sprintf(fr, "%s    SF%d BW%d", the_lora_config->plan,
      the_lora_config->sf, (int)(the_lora_config->bw/1000));
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 54, fr);
    */
    gXdisplay.setCursor(70, 202);
    gXdisplay.println("o - -");
  } else if (scr == UI_TWrist_Class::SCREEN_REPO) {
    gXdisplay.fillScreen(GxEPD_WHITE);
    gXdisplay.setCursor(60, 40);
    gXdisplay.println("repo");
    /*
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 3, tSSB_WIFI_SSID "-");
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(42, 0, my_ssid+8);
    theDisplay.setFont(ArialMT_Plain_10);

     if (r_time)
      theDisplay.drawString(0, 18, r_time);

    char stat_line[30];
    char gps_synced = 0;
# if !defined(NO_GPS)
    gps_synced = gps.location.isValid() ? 1 : 0;
#endif
    sprintf(stat_line, "W:%d E:%d G:%d L:%s",
            r_wifi_peers, r_ble_peers, r_gps_valid, wheel[r_lora_wheel % 4]);
    theDisplay.drawString(0, 30, stat_line);

    theDisplay.setFont(ArialMT_Plain_16);
    right_aligned(repo->rplca_cnt, 'F', 0); 
    right_aligned(repo->entry_cnt, 'E', 22); 
    right_aligned(repo->chunk_cnt, 'C', 44); 

    int total = MyFS.totalBytes();
    int avail = total - MyFS.usedBytes();
    char buf[10];
    sprintf(buf, "%2d%% free", avail / (total/100));
    theDisplay.drawString(0, 44, buf);
    */
    gXdisplay.setCursor(70, 202);
    gXdisplay.println("- o -");
  } else if (scr == UI_TWrist_Class::SCREEN_PEERS) {
    gXdisplay.fillScreen(GxEPD_WHITE);
    gXdisplay.setCursor(60, 40);
    gXdisplay.println("peers");
    /*
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString( 0, 0, "last");
    theDisplay.drawString(35, 0, "peer");
    theDisplay.drawString(65, 0, "rssi");
    theDisplay.drawString(95, 0, "snr");
    int y = 14;
    // theDisplay.setFont(ArialMT_Plain_10);
    long now = millis();
    for (int i = 0; i < MAX_PEERS; i++) {
      if (peers[i].id[0] == '\0')
        continue;
      theDisplay.drawString( 0, y, String((peers[i].when - now)/1000));
      theDisplay.drawString(35, y, peers[i].id);
      theDisplay.drawString(65, y, String(peers[i].rssi));
      theDisplay.drawString(95, y, String(peers[i].snr));
      y += 12;
    }
    */
    gXdisplay.setCursor(70, 202);
    gXdisplay.println("- - o");
  }

  gXdisplay.update();
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
