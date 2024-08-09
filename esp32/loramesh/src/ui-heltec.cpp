// ui-heltec.cpp

#ifdef TINYSSB_BOARD_HELTEC

#include <ctype.h>   // for toupper()
#include <cstdarg>   // for va_list()

#include "hardware.h"
#include "ui-heltec.h"


// user button
#define BUTTON_PIN KEY_BUILTIN  // for heltec?

#include "lib/cmd.h"

// ---------------------------------------------------------------------------

// Display

/*
enum OLEDDISPLAY_ANGLE { // taken from OLEDDisplay.h
  ANGLE_0_DEGREE = 0,
  ANGLE_90_DEGREE,
  ANGLE_180_DEGREE,
  ANGLE_270_DEGREE,
};

#include "ui-heltec/oled/SSD1306Wire.h"
*/
#include <SSD1306Wire.h>
SSD1306Wire *display;

#define theDisplay (*display)

// ---------------------------------------------------------------------------

#include <Button2.h>

Button2 userButton;
char lora_selection; // true if lora plan selection ongoing, else do_io
long selection_timeout;

void clicked(Button2& btn)
{
  // Serial.println("click");
  selection_timeout = millis() + 10000;

  if (lora_selection) { // cycle through the choices
#ifdef HAS_LORA
    int i = the_lora_config - lora_configs;
    i = (i+1) % lora_configs_cnt;
    the_lora_config = lora_configs + i;
    theUI->refresh();
#endif
  } else {
    ((UI_Heltec_Class*)theUI)->to_next_screen();
  }
}

void long_clicked(Button2& btn)
{
  // Serial.println("long_click");
  selection_timeout = millis() + 10000;

  if (lora_selection) { // this is our choice
#ifdef HAS_LORA
    struct bipf_s *v = bipf_mkString(the_lora_config->plan);
    bipf_dict_set(the_config, bipf_mkString("lora_plan"), v);
    config_save(the_config);

    char s[30];
    theDisplay.clear();
    theDisplay.setFont(ArialMT_Plain_24);
    sprintf(s, ">> %s", the_lora_config->plan);
    theDisplay.drawString(0, 10, s);
#endif
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0, 46, "rebooting now");
    theDisplay.display();

    delay(3000);
    // lora_log.printf(">> reboot cmd\n");
    // lora_log.close();
    esp_restart();
  } else {
    UI_Heltec_Class *u = (UI_Heltec_Class*) theUI;
    if ( u->curr_screen == UI_Heltec_Class::SCREEN_LORA) {
      lora_selection = 1 - lora_selection;
      u->refresh_screen(u->curr_screen);
    }
  }
}

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------

UIClass::UIClass()
{
  node_name = time = lora_profile = NULL;
  gps_lon = gps_lat = gps_ele = 0.0;
  f_cnt = e_cnt = c_cnt = 0;
  ble_cnt = wifi_cnt = 0;
  lora_fr = lora_bw = lora_sf = 0;

  // pinMode(BUTTON_PIN, INPUT);
  userButton.begin(BUTTON_PIN);
  userButton.setLongClickTime(1000);
  userButton.setClickHandler(clicked);
  userButton.setLongClickDetectedHandler(long_clicked);

  // OLED display
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, LOW);
  delay(50);
  digitalWrite(RST_OLED, HIGH);

  display = new SSD1306Wire(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);

  theDisplay.init();
  theDisplay.flipScreenVertically();
  theDisplay.setTextAlignment(TEXT_ALIGN_LEFT);

  theDisplay.clear();
  theDisplay.setFont(ArialMT_Plain_16);
  theDisplay.drawString(0, 0, "starting tinySSB");
  theDisplay.display();
}


UI_Heltec_Class::UI_Heltec_Class()
{

#define INFLATE(NM) inflate(NM##_bw, sizeof(NM##_bw),   \
                            NM##_z, sizeof(NM##_z));
  // INFLATE(scuttleshell)
  curr_screen = UI_Heltec_Class::SCREEN_SPLASH;

  wheel = "\\|/-";
  lora_wheel = 0;
}


void draw_lora_plan()
{
  theDisplay.setFont(ArialMT_Plain_24);
  theDisplay.drawString(0, 0, the_lora_config->plan);
  
  char s[30];
  theDisplay.setFont(ArialMT_Plain_16);
  int f = the_lora_config->fr / 10000;
  sprintf(s, "fr=%d.%02d MHz", f/100, f%100);
  theDisplay.drawString(0, 28, s);
  sprintf(s, "bw=%dkHz sf=%d",
          (int)(the_lora_config->bw/1000), the_lora_config->sf);
  theDisplay.drawString(0, 48, s);
}


void UI_Heltec_Class::loop()
{
  userButton.loop();

  if (lora_selection) {
    static char blink;
    static long next, timeout;

    if (millis() > next) {
      next = millis() + 100;
      blink = 1 - blink;

      if (selection_timeout < millis()) {
        lora_selection = 0;
        theUI->refresh();
      } else {
#if defined(HAS_OLED)
        theDisplay.clear();
        if (blink) {
          draw_lora_plan();
          next += 300;
        }
        theDisplay.display();
      }
    }
#endif
  }

  if (curr_screen == UI_Heltec_Class::SCREEN_SPLASH) {
    static double old_density;

    double d = thePeers->get_peer_density();
    if (d != old_density) {
      old_density = d;
      refresh_screen(curr_screen);
    }
  }
  
  if (curr_screen == UI_Heltec_Class::SCREEN_PEERS) {
    static long next;
    if (millis() > next) {
      refresh_screen(curr_screen);
      next = millis() + 900;
    }
  }
}


void UI_Heltec_Class::refresh()
{
  refresh_screen(curr_screen);
}


void UI_Heltec_Class::to_next_screen()
{
  curr_screen = (curr_screen + 1) % UI_Heltec_Class::SCREEN_OFF;
  refresh_screen(curr_screen);
}


void pos_key_val(int y, char *k, char *fmt, ...)
{
  va_list args;
  char buf[50];

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  /*
  gXdisplay.setCursor(0, y);
  gXdisplay.println(k);

  gXdisplay.setCursor(55, y);
  gXdisplay.println(buf);
  */
}


static void right_aligned(int cnt, char c, int y)
{
  char buf[20];
  sprintf(buf, "%d %c", cnt, c);
  int w = theDisplay.getStringWidth(buf);
  theDisplay.drawString(128-w, y, buf);
}


void UI_Heltec_Class::refresh_screen(int scr)
{
  if (scr != curr_screen)
      return;

  theDisplay.clear();
  theDisplay.setFont(ArialMT_Plain_16);
  if (scr == UI_Heltec_Class::SCREEN_SPLASH) {
    // gXdisplay.drawBitmap(gallery[pict], 0, 0, 200, 200, GxEPD_WHITE);
    // gXdisplay.setCursor(145, 11);
    // gXdisplay.println(ssid+7);
    // pict = (pict + 1) % (sizeof(gallery) / sizeof(unsigned char*));

    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0 , 0, "tinySSB");
    int w = theDisplay.getStringWidth(ssid+8);
    theDisplay.drawString(128-w, 0, ssid+8);
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0 , 17, __DATE__ " " __TIME__ UTC_OFFSET);

#ifdef HAS_LORA
    int f = the_lora_config->fr / 10000;
    char fr[30];
    sprintf(fr, "%d.%02d MHz", f/100, f%100);
    theDisplay.setFont(ArialMT_Plain_24);
    theDisplay.drawString(0, 30, fr);
    sprintf(fr, "%s   bw=%dkHz sf=%d", the_lora_config->plan,
            (int)(the_lora_config->bw/1000), the_lora_config->sf);
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 54, fr);

    double d = thePeers->get_peer_density();
    if (d >= 0.25) theDisplay.fillRect(72, 12, 3, 3);
    if (d >= 1.25) theDisplay.fillRect(76,  9, 3, 6);
    if (d >= 2.25) theDisplay.fillRect(80,  6, 3, 9);
    if (d >= 3.25) theDisplay.fillRect(84,  3, 3, 12);
#endif
  } else if (scr == UI_Heltec_Class::SCREEN_NODE) {
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString(0, 3, tSSB_WIFI_SSID "-");
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(42, 0, ssid+8);
    theDisplay.setFont(ArialMT_Plain_10);

    // if (r_time)
    //  theDisplay.drawString(0, 18, r_time);

    char stat_line[30];
    char gps_synced = 0;
#ifdef HAS_GPS
    gps_synced = gps.location.isValid() ? 1 : 0;
#endif
    sprintf(stat_line, "W:%d E:%d G:%d L:%c",
            wifi_cnt, ble_cnt, 0, wheel[lora_wheel % 4]);   // r_gps_valid
    theDisplay.drawString(0, 30, stat_line);

    theDisplay.setFont(ArialMT_Plain_16);
    if (theGOset->in_sync())
      right_aligned(theRepo->rplca_cnt, 'F', 0);
    else {
      char buf[20];
      sprintf(buf, "%d/%d", theGOset->goset_len, theGOset->largest_claim_span);
      int w = theDisplay.getStringWidth(buf);
      theDisplay.drawString(128-w, 0, buf);
    }
    right_aligned(theRepo->entry_cnt, 'E', 22); 
    right_aligned(theRepo->chunk_cnt, 'C', 44); 

    int total = MyFS.totalBytes();
    int avail = total - MyFS.usedBytes();
    char buf[10];
    sprintf(buf, "%2d%% free", avail / (total/100));
    theDisplay.drawString(0, 44, buf);
  } else if (scr == UI_Heltec_Class::SCREEN_LORA) {
#ifdef HAS_LORA
    draw_lora_plan();
#else
    theDisplay.drawString(2, 20, "lora screen");
#endif
  } else if (scr == UI_Heltec_Class::SCREEN_PEERS) {
    theDisplay.setFont(ArialMT_Plain_10);
    theDisplay.drawString( 0, 0, "last");
    theDisplay.drawString(35, 0, "peer");
    theDisplay.drawString(75, 0, "rssi");
    theDisplay.drawString(105, 0, "snr");
    int y = 14;
    long now = millis();
    for (int i = 0; i < MAX_HEARD_PEERS; i++) {
      struct peer_s *p = thePeers->heard_peers + i;
      if (p->id[0] == '\0')
        break;
      theDisplay.drawString( 0, y, String((p->when - now) / 1000));
      char buf[10];
      strcpy(buf, p->id); // uppercase is more readable
      for (int j=0; j < strlen(buf); j++) buf[j] = toupper(buf[j]);
      theDisplay.drawString(35, y, buf);
      theDisplay.drawString(75, y, String(p->rssi));
      sprintf(buf, "%.f1", p->snr);
      theDisplay.drawString(105, y, buf);
      y += 12;
    }
  }

  theDisplay.display();
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

int err_cnt;

void UI_Heltec_Class::show_boot_msg(char *s)
{
#ifdef HAS_OLED
#define MAX_BOOT_MSGS 4
  static char* boot_msgs[MAX_BOOT_MSGS];
  static short cnt;
  static short n = 1;

  if (cnt == MAX_BOOT_MSGS) {
    theDisplay.clear();
    theDisplay.setFont(ArialMT_Plain_16);
    theDisplay.drawString(0, 0, "starting tinySSB");
    memmove(boot_msgs, boot_msgs+1, 3*sizeof(char*));
    cnt--;
    n++;
    theDisplay.setFont(ArialMT_Plain_10);
    for (int i = 0; i < cnt; i++)
      theDisplay.drawString(0, 18 + i * 12, String(n+i) + "> " + String(boot_msgs[i]));
  }
  boot_msgs[cnt] = s;
  theDisplay.setFont(ArialMT_Plain_10);
  theDisplay.drawString(0, 18 + cnt * 12, String(n+cnt) + "> " + String(s));
  theDisplay.display();
  cnt++;
#endif
}


void UI_Heltec_Class::lora_advance_wheel()
{
  lora_wheel++;
  refresh();
};

#endif // TINYSSB_BOARD_HELTEC

// eof
