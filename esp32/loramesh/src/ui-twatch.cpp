// ui-twatch.cpp

#include "ui-twatch.h"

#if defined(TINYSSB_BOARD_TWATCH)

#include "lib/tinySSBlib.h"

#include "hardware.h"
#include "lib/cmd.h"


// #include <SPI.h>
// #include <TFT_eSPI.h>
// #include "es7210.h"
// #include <Audio.h>
// #include <driver/i2s.h>

// TFT_eSPI    tft;

// #define TOUCH_MODULES_GT911
// #include "TouchLib.h"
// #include "AceButton.h"

// using namespace ace_button;
// AceButton   button;
// bool        clicked = false;

// ---------------------------------------------------------------------------

#if 0

TouchLib *touch = NULL;
uint8_t   touchAddress = GT911_SLAVE_ADDRESS2;

lv_indev_t  *touch_indev = NULL;
lv_indev_t  *kb_indev = NULL;

SemaphoreHandle_t xSemaphore = NULL;


static void disp_flush( lv_disp_drv_t *disp,
                        const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        tft.startWrite();
        tft.setAddrWindow( area->x1, area->y1, w, h );
        tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
        tft.endWrite();
        lv_disp_flush_ready( disp );
        xSemaphoreGive( xSemaphore );
    }
}


static bool getTouch(int16_t &x, int16_t &y)
{
    uint8_t rotation = tft.getRotation();
    if (!touch->read()) {
        return false;
    }
    TP_Point t = touch->getPoint(0);
    switch (rotation) {
    case 1:
        x = t.y;
        y = tft.height() - t.x;
        break;
    case 2:
        x = tft.width() - t.x;
        y = tft.height() - t.y;
        break;
    case 3:
        x = tft.width() - t.y;
        y = t.x;
        break;
    case 0:
    default:
        x = t.x;
        y = t.y;
    }
    // Serial.printf("R:%d X:%d Y:%d\r\n", rotation, x, y);
    return true;
}

// Read the touchpad
static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    data->state = getTouch(data->point.x, data->point.y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

// Read key value from esp32c3
static uint32_t keypad_get_key(void)
{
    char key_ch = 0;
    Wire.requestFrom(0x55, 1);
    while (Wire.available() > 0) {
        key_ch = Wire.read();
    }
    return key_ch;
}


// Will be called by the library to read the mouse
static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint32_t last_key = 0;
    uint32_t act_key ;
    act_key = keypad_get_key();
    if (act_key != 0) {
        data->state = LV_INDEV_STATE_PR;
        Serial.printf("Key pressed : 0x%x\n", act_key);
        last_key = act_key;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    data->key = last_key;
}


static void setupLvgl()
{
    static lv_disp_draw_buf_t draw_buf;

#ifndef BOARD_HAS_PSRAM
#define LVGL_BUFFER_SIZE    ( TFT_HEIGHT * 100 )
    static lv_color_t buf[ LVGL_BUFFER_SIZE ];
#else
#define LVGL_BUFFER_SIZE    (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
    static lv_color_t *buf = (lv_color_t *) ps_malloc(LVGL_BUFFER_SIZE);
    if (!buf) {
      Serial.printf("PSRAM malloc for LVGL failed: free=%d %d %d %d, trying standard RAM\r\n",
                    ESP.getFreeHeap(),
                    LVGL_BUFFER_SIZE, TFT_WIDTH, TFT_HEIGHT);
      buf = (lv_color_t *) malloc(LVGL_BUFFER_SIZE);    
      delay(5000);
      assert(buf);
    }
#endif
    memset(buf, 0, LVGL_BUFFER_SIZE);

    String LVGL_Arduino = String("LVLG Arduino V") + lv_version_major() +
                          "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println( LVGL_Arduino );

    lv_init();
    lv_group_set_default(lv_group_create());
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUFFER_SIZE );

    // Initialize the display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );

    // Change the following line to your display resolution
    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef BOARD_HAS_PSRAM
    disp_drv.full_refresh = 1;
#endif
    lv_disp_drv_register( &disp_drv );

    /*Initialize the input device driver*/

    /*Register a touchscreen input device*/
    Wire.beginTransmission(touchAddress);
    if (Wire.endTransmission() == 0) {
        static lv_indev_drv_t indev_touchpad;
        lv_indev_drv_init( &indev_touchpad );
        indev_touchpad.type = LV_INDEV_TYPE_POINTER;
        indev_touchpad.read_cb = touchpad_read;
        touch_indev = lv_indev_drv_register( &indev_touchpad );
    }

    Wire.requestFrom(0x55, 1);
    if (Wire.read() != -1) {
        // Serial.println("Keyboard registered!!");
        // Register a keypad input device
        static lv_indev_drv_t indev_keypad;
        lv_indev_drv_init(&indev_keypad);
        indev_keypad.type = LV_INDEV_TYPE_KEYPAD;
        indev_keypad.read_cb = keypad_read;
        kb_indev = lv_indev_drv_register(&indev_keypad);
        lv_indev_set_group(kb_indev, lv_group_get_default());
    }

}


static void scanDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    // uint32_t start = 0;
    for (addr = 1; addr < 127; addr++) {
        // start = millis();
        w->beginTransmission(addr);
        delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            /*
            Serial.print("I2C device found at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.print(addr, HEX);
            Serial.println(" !");
            */
            if (addr == GT911_SLAVE_ADDRESS2) {
                touchAddress = GT911_SLAVE_ADDRESS2;
                Serial.println("Touchpad - found GT911 Drv Slave address: 0x14");
            } else if (addr == GT911_SLAVE_ADDRESS1) {
                touchAddress = GT911_SLAVE_ADDRESS1;
                Serial.println("Touchpad - found GT911 Drv Slave address: 0x5D");
            }
        } else if (err == 4) {
          /*
            Serial.print("Unknow error at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.println(addr, HEX);
          */
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
}


static void handleEvent(AceButton * /* button */, uint8_t eventType,
                        uint8_t /* buttonState */)
{
    switch (eventType) {
    case AceButton::kEventClicked:
        clicked = true;
        Serial.println("Clicked!");
        break;
    case AceButton::kEventLongPressed:

        Serial.println("ClickkEventLongPresseded!"); delay(2000);

#if TFT_BL !=  BOARD_BL_PIN
#error "Not using the already configured T-Deck file, please remove <Arduino/libraries/TFT_eSPI> and replace with <lib/TFT_eSPI>, please do not click the upgrade library button when opening sketches in ArduinoIDE versions 2.0 and above, otherwise the original configuration file will be replaced !!!"
#error "Not using the already configured T-Deck file, please remove <Arduino/libraries/TFT_eSPI> and replace with <lib/TFT_eSPI>, please do not click the upgrade library button when opening sketches in ArduinoIDE versions 2.0 and above, otherwise the original configuration file will be replaced !!!"
#error "Not using the already configured T-Deck file, please remove <Arduino/libraries/TFT_eSPI> and replace with <lib/TFT_eSPI>, please do not click the upgrade library button when opening sketches in ArduinoIDE versions 2.0 and above, otherwise the original configuration file will be replaced !!!"
#endif


        //If you need other peripherals to maintain power, please set the IO port to hold
        // gpio_hold_en((gpio_num_t)BOARD_POWERON);
        // gpio_deep_sleep_hold_en();

        // When sleeping, set the touch and display screen to sleep, and all other peripherals will be powered off
        pinMode(BOARD_TOUCH_INT, OUTPUT);
        digitalWrite(BOARD_TOUCH_INT, LOW); //Before touch to set sleep, it is necessary to set INT to LOW
        touch->enableSleep();        //set touchpad enter sleep mode
        tft.writecommand(0x10);     //set disaplay enter sleep mode
        delay(2000);
        esp_sleep_enable_ext1_wakeup(1ull << BOARD_BOOT_PIN, ESP_EXT1_WAKEUP_ALL_LOW);
        esp_deep_sleep_start();
        //Deep sleep consumes approximately 240uA of current
        break;
    }
}
#endif // 0

// ---------------------------------------------------------------------------

UIClass::UIClass()
{
  Serial.println("init of UIClass()");

#if 0
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

  button.init();
  ButtonConfig *buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);

  //Add mutex to allow multitasking access
  xSemaphore = xSemaphoreCreateBinary();
  assert(xSemaphore);
  xSemaphoreGive( xSemaphore );

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);
  // Set touch int input
  pinMode(BOARD_TOUCH_INT, INPUT); delay(20);

  // Two touch screens, the difference between them is the device address,
  // use ScanDevices to get the existing I2C address
  scanDevices(&Wire);

  touch = new TouchLib(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, touchAddress);
  touch->init();
  Wire.beginTransmission(touchAddress);
  Wire.endTransmission(); // return code confirms touch

  Wire.requestFrom(0x55, 1);
  Wire.read(); // != -1;  return code confirms keyboard

  setupLvgl();
#endif // 0
}

#if 0
void UI_TWatch_Class::spinner(bool show)
{
  if (show)
    lv_obj_clear_flag(spin, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(spin, LV_OBJ_FLAG_HIDDEN);
  lv_task_handler();
}

void UI_TWatch_Class::to_next_screen() {}

void UI_TWatch_Class::buzz() {}

void UI_TWatch_Class::loop()
{
  lv_task_handler();
}
#endif // 0

// ---------------------------------------------------------------------------

extern UIClass *theUI;

#if 0
static void brightness_cb(lv_event_t *e)
{
  ((UI_TWatch_Class*)theUI)->_brightness_cb(e);
}
static void four_button_cb(lv_event_t *e)
{
  ((UI_TWatch_Class*)theUI)->_four_button_cb(e);
}

#include "../assets/tdeck/splash.h"
#include "../assets/tdeck/tremola.h"

#include "../assets/tdeck/chat.h"
#include "../assets/tdeck/chat_checked.h"
#include "../assets/tdeck/contacts.h"
#include "../assets/tdeck/contacts_checked.h"
#include "../assets/tdeck/haze0.h"
#include "../assets/tdeck/haze1.h"
#include "../assets/tdeck/haze2.h"
#include "../assets/tdeck/menu.h"
#include "../assets/tdeck/return_btn.h"
#include "../assets/tdeck/signal_btn.h"
#include "../assets/tdeck/signal_btn_checked.h"
#include "../assets/tdeck/tab_left.h"
#include "../assets/tdeck/tab_right.h"

#endif

void UI_TWatch_Class::refresh()
{
  // lv_obj_invalidate(scr);
  // lv_task_handler();
}

#if 0

void UI_TWatch_Class::_brightness_cb(lv_event_t *e)
{
  static char lvl;

  lvl = (lvl + 1) % 4;
  if (lvl == 0) {
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hz[2], LV_OBJ_FLAG_HIDDEN);
  } else if (lvl == 1) {
    lv_obj_clear_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
  } else if (lvl == 2) {
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hz[2], LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_invalidate(hz[0]);
  lv_obj_invalidate(hz[1]);
  lv_obj_invalidate(hz[2]);
}


void UI_TWatch_Class::_four_button_cb(lv_event_t *e)
{
  lv_obj_t *t = lv_event_get_target(e);
  // void *aux = lv_event_get_user_data(e);

  if (current_btn == t)
    return;

  // lv_event_stop_bubbling(e);
  for (int i = 0; i < 4; i++) {
    lv_obj_clear_state(four_buttons[i],
                       LV_STATE_CHECKED | LV_STATE_PRESSED);
    if (t == four_buttons[i])
      current_btn = t;
    else
      lv_obj_add_state(four_buttons[i], LV_STATE_CHECKED);
    lv_obj_invalidate(four_buttons[i]);
  }
  lv_task_handler();
}


/*

void prep2log(char *txt) // useful for showing internal log messages on the GUI
{
  char *olds = lv_label_get_text(log_lbl);
  char *news = (char*) malloc(strlen(olds) + strlen(txt) + 8);
  strcpy(news, txt);
  strcpy(news + strlen(news), "\n---\n");
  strcpy(news + strlen(news), olds);
  int i = strlen(news);
  if (i > 1024)
    news[1024] = '\0';
  lv_label_set_text(log_lbl, news);
  free(news);
  lv_obj_invalidate(log_lbl);
}

*/

// eof
#endif // 0

UI_TWatch_Class::UI_TWatch_Class()
{
#if 0
  
  // Serial.println("init of UI_TWatch_Class()");
  
    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_style_set_text_color(&bg_style, lv_color_hex(0xff4040));

    scr = lv_scr_act();
    lv_obj_t *tmp;
    
    // splash

    lv_obj_t *img = lv_img_create(scr); // background
    lv_img_set_src(img, &splash);
    lv_obj_set_size(img, 320, 240);
    lv_obj_set_pos(img, 0, 0);
    
    lv_obj_invalidate(img);
    uint32_t run = millis() + 1000;
    while (millis() < run) {
        lv_task_handler();
        delay(5);
    }
    lv_obj_del(img);

    // background
    
    img = lv_img_create(scr); // background
    lv_img_set_src(img, &tremola);
    lv_obj_set_pos(img, 0, 0);
    lv_obj_invalidate(img);
    lv_task_handler();

    // posts (and scroll)

    posts = lv_obj_create(img);
    lv_obj_set_size(posts, 314, 160);
    lv_obj_set_pos(posts, 3, 36);
    lv_obj_set_style_pad_all(posts, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(posts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(posts, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(posts, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(posts, LV_FLEX_FLOW_COLUMN);

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    log_lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(log_lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_obj_set_style_text_font(log_lbl, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(log_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(log_lbl, "-- top of posts --");

    // top row (left and right button, middle title)

    btn_left = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(btn_left, 0, LV_PART_MAIN);
    lv_obj_set_size(btn_left, 30, 30);
    lv_obj_set_pos(btn_left, 3, 3);
    lv_imgbtn_set_src(btn_left, LV_IMGBTN_STATE_RELEASED, 0, &return_btn, 0);
    lv_obj_add_event_cb(btn_left, brightness_cb, LV_EVENT_CLICKED, NULL);

    bar = lv_label_create(img);
    lv_obj_set_size(bar, LV_HOR_RES - 38 - 38, 30);
    lv_obj_set_pos(bar, 38, 3);
    lv_obj_add_style(bar, &bg_style, LV_PART_MAIN);
    lv_label_set_text(bar, "some title here\nand more on the next line");

    btn_right = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(btn_right, 0, LV_PART_MAIN);
    lv_obj_set_size(btn_right, 30, 30);
    lv_obj_set_pos(btn_right, LV_HOR_RES - 33, 3);
    lv_imgbtn_set_src(btn_right, LV_IMGBTN_STATE_RELEASED, 0, &menu, 0);
    
    // four "tab" buttons

    lv_obj_t *ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 0, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &chat, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &chat_checked, 0);
    four_buttons[0] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 80, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &contacts, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &contacts_checked, 0);
    four_buttons[1] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 160, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &signal_btn, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &signal_btn_checked, 0);
    four_buttons[2] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 240, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &chat, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &chat_checked, 0);
    four_buttons[3] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    // "haze" (poor man's way of dimming the screen)
 
    hz[0] = lv_img_create(scr);
    lv_img_set_src(hz[0], &haze0);
    lv_obj_set_size(hz[0], 320, 240);
    lv_obj_set_pos(hz[0], 0, 0);
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);

    hz[1] = lv_img_create(scr);
    lv_img_set_src(hz[1], &haze1);
    lv_obj_set_size(hz[1], 320, 240);
    lv_obj_set_pos(hz[1], 0, 0);
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);

    hz[2] = lv_img_create(scr);
    lv_img_set_src(hz[2], &haze2);
    lv_obj_set_size(hz[2], 320, 240);
    lv_obj_set_pos(hz[2], 0, 0);
    lv_obj_add_flag(hz[2], LV_OBJ_FLAG_HIDDEN);

    // final refresh

    spin = lv_spinner_create(lv_scr_act(), 1500, 60);
    lv_obj_add_style(spin, &bg_style, LV_PART_ITEMS); // FIXME: no effect ...
    lv_obj_set_size(spin, 60, 60);
    lv_obj_center(spin);
    lv_obj_add_flag(spin, LV_OBJ_FLAG_HIDDEN);

    refresh();
#endif // 0
}

#endif // TINYSSB_BOARD_TWATCH
// eof
