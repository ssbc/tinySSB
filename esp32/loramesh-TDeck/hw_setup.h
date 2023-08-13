// hw_setup.h


using namespace ace_button;
AceButton   button;
bool        clicked = false;

TouchLib *touch = NULL;
uint8_t   touchAddress = GT911_SLAVE_ADDRESS2;

TFT_eSPI        tft;

lv_indev_t  *touch_indev = NULL;
lv_indev_t  *kb_indev = NULL;

SemaphoreHandle_t xSemaphore = NULL;

// ---------------------------------------------------------------------------

static void scanDevices(TwoWire *w)
{
    uint8_t err, addr;
    int nDevices = 0;
    uint32_t start = 0;
    for (addr = 1; addr < 127; addr++) {
        start = millis();
        w->beginTransmission(addr); delay(2);
        err = w->endTransmission();
        if (err == 0) {
            nDevices++;
            Serial.print("I2C device found at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.print(addr, HEX);
            Serial.println(" !");

            if (addr == GT911_SLAVE_ADDRESS2) {
                touchAddress = GT911_SLAVE_ADDRESS2;
                Serial.println("Find GT911 Drv Slave address: 0x14");
            } else if (addr == GT911_SLAVE_ADDRESS1) {
                touchAddress = GT911_SLAVE_ADDRESS1;
                Serial.println("Find GT911 Drv Slave address: 0x5D");
            }
        } else if (err == 4) {
            Serial.print("Unknow error at address 0x");
            if (addr < 16) {
                Serial.print("0");
            }
            Serial.println(addr, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
}


void handleEvent(AceButton * /* button */, uint8_t eventType,
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


void initBoard()
{
    bool ret = 0;

    Serial.begin(115200);
    Serial.println("T-DECK factory");

    //! The board peripheral power control pin needs to be set to HIGH when using the peripheral
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
    tft.setRotation( 1 );
    tft.fillScreen(TFT_BLACK);
    /*
    tft.writecommand(0xBC);
    tft.writedata(0x40);     //contrast value
    tft.writedata(0x20);     //brightness value
    tft.writedata(0x40);     //saturation value
    tft.writedata(0x01);     //Post Processor Enable
    */
    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);


    // Set touch int input
    pinMode(BOARD_TOUCH_INT, INPUT); delay(20);

    // Two touch screens, the difference between them is the device address,
    // use ScanDevices to get the existing I2C address
    scanDevices(&Wire);

    touch = new TouchLib(Wire, BOARD_I2C_SDA, BOARD_I2C_SCL, touchAddress);
    touch->init();

    pinMode(BOARD_BOOT_PIN, INPUT);

    while (!digitalRead(BOARD_BOOT_PIN)) {
        Serial.println("BOOT HAS PRESSED!!!"); delay(500);
    }

    /*
    if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        if (hasRadio) {
            if (sender) {
                transmissionState = radio.startTransmit("0");
                sendCount = 0;
                Serial.println("startTransmit!!!!");
            } else {
                int state = radio.startReceive();
                if (state == RADIOLIB_ERR_NONE) {
                    Serial.println(F("success!"));
                } else {
                    Serial.print(F("failed, code "));
                    Serial.println(state);
                }
            }
        }
        xSemaphoreGive( xSemaphore );
    }
    */
}


bool setupSD()
{
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);

    if (SD.begin(BOARD_SDCARD_CS, SPI, 800000U)) {
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("No SD_MMC card attached");
            return false;
        } else {
            Serial.print("SD_MMC Card Type: ");
            if (cardType == CARD_MMC) {
                Serial.println("MMC");
            } else if (cardType == CARD_SD) {
                Serial.println("SDSC");
            } else if (cardType == CARD_SDHC) {
                Serial.println("SDHC");
            } else {
                Serial.println("UNKNOWN");
            }
            uint32_t cardSize = SD.cardSize() / (1024 * 1024);
            uint32_t cardTotal = SD.totalBytes() / (1024 * 1024);
            uint32_t cardUsed = SD.usedBytes() / (1024 * 1024);
            Serial.printf("SD Card Size: %lu MB\n", cardSize);
            Serial.printf("Total space: %lu MB\n",  cardTotal);
            Serial.printf("Used space: %lu MB\n",   cardUsed);
            return true;
        }
    }
    return false;
}


static void disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    // if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        tft.startWrite();
        tft.setAddrWindow( area->x1, area->y1, w, h );
        tft.pushColors( ( uint16_t * )&color_p->full, w * h, false );
        tft.endWrite();
        lv_disp_flush_ready( disp );
    //    xSemaphoreGive( xSemaphore );
    // }
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
    Serial.printf("R:%d X:%d Y:%d\n", rotation, x, y);
    return true;
}

/*Read the touchpad*/
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


/*Will be called by the library to read the mouse*/
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


void setupLvgl()
{
    static lv_disp_draw_buf_t draw_buf;

#ifndef BOARD_HAS_PSRAM
#define LVGL_BUFFER_SIZE    ( TFT_HEIGHT * 100 )
    static lv_color_t buf[ LVGL_BUFFER_SIZE ];
#else
#define LVGL_BUFFER_SIZE    (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
    static lv_color_t *buf = (lv_color_t *)ps_malloc(LVGL_BUFFER_SIZE);
    if (!buf) {
        Serial.println("menory alloc failed!");
        delay(5000);
        assert(buf);
    }
#endif
    memset(buf, 0, LVGL_BUFFER_SIZE);

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println( LVGL_Arduino );
    Serial.println( "I am LVGL_Arduino" );

    lv_init();
    lv_group_set_default(lv_group_create());
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUFFER_SIZE );

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );

    /*Change the following line to your display resolution*/
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

// ---------------------------------------------------------------------------

void hw_setup()
{
  initBoard();
  setupSD();
  setupLvgl();
}

// eof
