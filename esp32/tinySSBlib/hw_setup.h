// hw_setup.h



unsigned char my_mac[6];

#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
char ssid[sizeof(tSSB_WIFI_SSID) + 6];
#endif

#if defined(HAS_BT)
BluetoothSerial BT;
#endif

// ---------------------------------------------------------------------------


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
