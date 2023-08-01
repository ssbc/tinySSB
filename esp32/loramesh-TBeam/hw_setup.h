// hw_dsetup.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// collect all external libraries here
// ...

// create instances

#include "config.h"

#if !defined(NO_WIFI)
  WiFiUDP udp;
#endif
char ssid[sizeof(tSSB_WIFI_SSID) + 6];
int wifi_clients = 0;

IPAddress broadcastIP;
#if !defined(NO_BT)
  BluetoothSerial BT;
#endif

#if defined(AXP_DEBUG)
AXP20X_Class axp;
#endif

#if defined(LORA_LOG)
unsigned long int next_log_battery;
#endif

#if !defined(NO_GPS)
TinyGPSPlus gps;
HardwareSerial GPS(1);
#endif

Button2 userButton;
unsigned char my_mac[6];

// -------------------------------------------------------------------

#if defined(SLOW_CPU)
  void cpu_set_slow() { setCpuFrequencyMhz(SLOW_CPU_MHZ); }
  void cpu_set_fast() { setCpuFrequencyMhz(240); }
#else
# define cpu_set_slow() ;
# define cpu_set_fast() ;
#endif

// -------------------------------------------------------------------

static unsigned char OLED_state = HIGH;

void OLED_toggle() {
  OLED_state = OLED_state ? LOW : HIGH;
#if !defined(NO_OLED)
  if (OLED_state == LOW) theDisplay.displayOff();
  else                   theDisplay.displayOn();
#endif
}

void pressed(Button2& btn) {
  // Serial.println("pressed");
  OLED_toggle();
}

// -------------------------------------------------------------------


void hw_setup() // T-BEAM or Heltec LoRa32v2
{
  the_config = config_load();
  the_lora_config = lora_configs;
  struct bipf_s k = { BIPF_STRING, 9, {.str = "lora_plan"} };
  struct bipf_s *lora_ref = bipf_dict_getref(the_config, &k);
  if (lora_ref != NULL && lora_ref->typ == BIPF_STRING) {
    for (int i = 0; i < lora_configs_size; i++)
      if (!strncmp(lora_configs[i].plan, lora_ref->u.str, lora_ref->cnt)) {
        the_lora_config = lora_configs + i;
        break;
      }
  }
  
  // Serial.begin(BAUD_RATE);

#if defined(WIFI_LoRa_32_V2) || defined(WIFI_LORA_32_V2)
  Heltec.begin(true /*DisplayEnable Enable*/,
               true /*Heltec.Heltec.Heltec.LoRa Disable*/,
               true /*Serial Enable*/,
               true /*PABOOST Enable*/,
               the_lora_config->fr);
  LoRa.setTxPower(the_lora_config->tx, RF_PACONFIG_PASELECT_PABOOST);
  
#else // T-Beam
  while (!Serial);
  delay(100);

#if !defined(NO_OLED)
  theDisplay.init();
  theDisplay.flipScreenVertically();
  theDisplay.setFont(ArialMT_Plain_10);
  theDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
#endif

  /*
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  // digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high、
  OLED_toggle();
  */

#if defined(HAS_LORA)
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);  
  if (!LoRa.begin(the_lora_config->fr)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(the_lora_config->tx);
#endif

  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    // Serial.println("AXP192 Begin PASS");
#if !defined(HAS_LORA)
    axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
#else
    axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
#endif
#if defined(NO_GPS)
    axp.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
#else
    axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
#endif
    axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
#if defined(NO_OLED)
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_OFF); // OLED
#else
    axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); // OLED
#endif
#if !defined(NO_GPS)
    GPS.begin(9600, SERIAL_8N1, 34, 12);   //17-TX 18-RX
#endif
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
#endif // T-Beam

#if defined(HAS_LORA)
  LoRa.setSignalBandwidth(the_lora_config->bw);
  LoRa.setSpreadingFactor(the_lora_config->sf);
  LoRa.setCodingRate4(the_lora_config->cr);
  LoRa.setPreambleLength(8);
  LoRa.setSyncWord(the_lora_config->sw);
  // LoRa.onReceive(newLoRaPkt);
  LoRa.receive();
#endif

  // pinMode(BUTTON_PIN, INPUT);
  userButton.begin(BUTTON_PIN);
  userButton.setPressedHandler(pressed);

  Serial.println("\n** Starting Scuttlebutt vPub (LoRa, WiFi, BLE) with GOset **\n");
#if !defined(NO_OLED)
  theDisplay.clear();
  theDisplay.display();
#endif


  // -------------------------------------------------------------------
  if (!MyFS.begin(true)) { // FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("LittleFS Mount Failed, partition was reformatted");
    // return;
  }
  // MyFS.format(); // uncomment and run once after a change in partition size

  // -------------------------------------------------------------------

  // esp_read_mac(my_mac, ESP_MAC_WIFI_STA);
  esp_efuse_mac_get_default(my_mac);
#if defined(NO_WIFI)  // in case of no Wifi: display BT mac addr, instead
  my_mac[5] += 2;
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.0/api-reference/system/base_mac_address.html
#endif
  Serial.println(String("mac   ") + to_hex(my_mac, 6, 1));
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2, 0));

#if !defined(NO_WIFI)
  WiFi.disconnect(true);
  delay(500);
  WiFi.mode(WIFI_AP);
  Serial.println(String("wifi  ") + ssid + " / " + tSSB_WIFI_PW);

  WiFi.softAP(ssid, tSSB_WIFI_PW, 7, 0, 4); // limit to four clients
  broadcastIP.fromString(tSSB_UDP_ADDR);
  if (!udp.beginMulticast(broadcastIP, tSSB_UDP_PORT)) {
    Serial.println("could not create multicast socket");
  } else {
    Serial.println("udp   " + broadcastIP.toString() + " / " + String(tSSB_UDP_PORT));
  }
#endif

#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)
  ble_init();
#endif

#if !defined(NO_BT)
  BT.begin(ssid);
  BT.setPin("0000");
  BT.write(KISS_FEND);
#endif

  Serial.println();
}

// --------------------------------------------------------------------------

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\r\n", file.name());
      if (levels)
        listDir(fs, file.path(), levels -1);
    } else
      Serial.printf("  FILE: %s\tSIZE: %d\r\n", file.name(), file.size());
    file = root.openNextFile();
  }
}

// eof
