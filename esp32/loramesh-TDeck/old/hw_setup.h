// hw_setup.h


using namespace ace_button;
AceButton   button;
bool        clicked = false;

TouchLib *touch = NULL;
uint8_t   touchAddress = GT911_SLAVE_ADDRESS2;

#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                          RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

TFT_eSPI    tft;

lv_indev_t  *touch_indev = NULL;
lv_indev_t  *kb_indev = NULL;

SemaphoreHandle_t xSemaphore = NULL;

unsigned char my_mac[6];

#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
char ssid[sizeof(tSSB_WIFI_SSID) + 6];
#endif

#if defined(HAS_BT)
BluetoothSerial BT;
#endif

// ---------------------------------------------------------------------------

void tft_onOff(int val)
{
  digitalWrite(BOARD_BL_PIN, val ? 1 : 0);
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
  /*
    digitalWrite(BOARD_SDCARD_CS, HIGH);
    digitalWrite(RADIO_CS_PIN, HIGH);
    digitalWrite(BOARD_TFT_CS, HIGH);
  */
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


void setupLvgl()
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

// ---------------------------------------------------------------------------

void setupBT()
{
#if defined(HAS_BT)
  BT.begin(ssid);
  BT.setPin("0000");
  BT.write(KISS_FEND);
#endif // HAS_BT
}

void loopBLE()
{
#if defined(MAIN_BLEDevice_H_) && !defined(NO_BLE)
  unsigned char *cp = ble_fetch_received();
  if (cp != NULL)
    incoming(&ble_face, cp+1, *cp, 0);
#endif
}

// ---------------------------------------------------------------------------
int lora_ok;

void setupLoRa()
{
#if defined(HAS_LORA)

#if defined(USE_RADIO_LIB)

  /*
  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);
  */
  SPI.end();
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD

  lora_ok = radio.begin(the_lora_config->fr / 1000000.0) == RADIOLIB_ERR_NONE;
  if (!lora_ok)
    Serial.println("RadioLib LoRa init failed");
  else {
    // radio.setFrequency(the_lora_config->fr/1000000.0);
    radio.setBandwidth((double)(the_lora_config->bw));
    radio.setSpreadingFactor(the_lora_config->sf);
    radio.setCodingRate(the_lora_config->cr);
    radio.setSyncWord(the_lora_config->sw);
    radio.setOutputPower(the_lora_config->tx);
    radio.setCurrentLimit(140); // (accepted range is 45 - 140 mA), 0=disable
    radio.setPreambleLength(8); // (accepted range is 0 - 65535)
    radio.setCRC(false);
    
    // set the function that will be called
    // when new packet is received
    radio.setPacketReceivedAction(newLoraPacket_cb);
    
    int rc = radio.startReceive();
    if (rc != RADIOLIB_ERR_NONE)
      Serial.printf("radio.startReceive() returned %d\r\n", rc);

    Serial.printf("LoRa configured for fr=%d, bw=%d, sf=%d\r\n",
                the_lora_config->fr, the_lora_config->bw, the_lora_config->sf);
  }
  
# else

  /*
  delay(4000);
  digitalWrite(BOARD_SDCARD_CS, HIGH);
  digitalWrite(RADIO_CS_PIN, HIGH);
  digitalWrite(BOARD_TFT_CS, HIGH);

  SPI.end();
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI, RADIO_CS_PIN);

  SPI.end();
  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI); //SD
  */

  pinMode(RADIO_RST_PIN, OUTPUT);
  digitalWrite(RADIO_RST_PIN, LOW);
  delay(100);
  digitalWrite(RADIO_RST_PIN, HIGH);
  
  lora_ok = false;
  delay(3000);
  LoRa.setSPI(SPI);
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO1_PIN);
  // LoRa.setPins(9, 17, 45);  //Meshtastic
  if (LoRa.begin(the_lora_config->fr)) {
    lora_ok = true;
  }
  if (lora_ok) {
      LoRa.setSignalBandwidth(the_lora_config->bw);
      LoRa.setSpreadingFactor(the_lora_config->sf);
      LoRa.setCodingRate4(the_lora_config->cr);
      LoRa.setPreambleLength(8);
      LoRa.setSyncWord(the_lora_config->sw);
      LoRa.setTxPower(the_lora_config->tx);
      // LoRa.onReceive(newLoRaPkt);
      LoRa.receive();
      Serial.printf("LoRa configured for fr=%d, bw=%d, sf=%d\r\n",
               the_lora_config->fr, the_lora_config->bw, the_lora_config->sf);
      Serial.flush();
  } else
    Serial.println("LoRa init failed");

#endif

#endif // HAS_LORA
}

// ---------------------------------------------------------------------------

void hw_setup()
{
  /*
  Serial.printf("Total heap: %d\r\n", ESP.getHeapSize());
  Serial.printf("Free heap: %d\r\n", ESP.getFreeHeap());
  Serial.printf("Total PSRAM: %d\r\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %d\r\n", ESP.getFreePsram());
  */
  
  esp_efuse_mac_get_default(my_mac);

#if !defined(HAS_UDP)  // in case of no Wifi: display BT mac addr, instead
  my_mac[5] += 2;
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.0/api-reference/system/base_mac_address.html
#endif
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2));
  Serial.printf("This is node %s\r\n\r\n", ssid);
#endif

  initBoard();

  if (!MyFS.begin(true)) {
    Serial.println("LittleFS Mount Failed, partition was reformatted");
    // return;
  }
  // MyFS.format(); // uncomment and run once after a change in partition size
  MyFS.mkdir(FEED_DIR);
  Serial.printf("LittleFS: %d total bytes, %d used\r\n",
                MyFS.totalBytes(), MyFS.usedBytes());

  the_config = config_load();
  struct bipf_s k = { BIPF_STRING, 9, {.str = "lora_plan"} };
  struct bipf_s *v = bipf_dict_getref(the_config, &k);
  /*
  if (v != NULL && v->typ == BIPF_STRING &&
                                    strncmp("US902.5", v->u.str, v->cnt)) {
    bipf_dict_set(the_config, bipf_mkString("lora_plan"),
                  bipf_mkString("US902.5"));
    config_save(the_config);
  }
  */

  // FIXME: we should not print the mgmt signing key to the console ?
  Serial.printf("tinySSB config is %s\r\n",
                bipf2String(the_config, "\r\n", 1).c_str());
  Serial.println();
  
  the_lora_config = lora_configs;
  // struct bipf_s k = { BIPF_STRING, 9, {.str = "lora_plan"} };
  struct bipf_s *lora_ref = bipf_dict_getref(the_config, &k);
  if (lora_ref != NULL && lora_ref->typ == BIPF_STRING) {
    for (int i = 0; i < lora_configs_size; i++)
      if (!strncmp(lora_configs[i].plan, lora_ref->u.str, lora_ref->cnt)) {
        the_lora_config = lora_configs + i;
        break;
      }
  }

  setupSD();
  setupLvgl();

  setupBT();
  setupLoRa();
  io_init();

  Serial.println("-- hw_setup done");
}

// eof
