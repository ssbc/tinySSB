// loramesh-TWrist.h

#include "config.h"

GxIO_Class gXio(SPI, /*CS*/ EPD_CS, /*DC=*/EPD_DC, /*RST=*/EPD_RESET);
GxEPD_Class gXdisplay(gXio, /*RST=*/EPD_RESET, /*BUSY=*/EPD_BUSY);

unsigned char my_mac[6];
char ssid[sizeof(tSSB_WIFI_SSID) + 6];
DmxClass   *theDmx;

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

void setup()
{
  Serial.begin(115200);
  delay(10);

  Serial.println("\r\ntinySSB virtual pub for the Lilygo T-Wrist device");

  SPI.begin(SPI_SCK, -1, SPI_DIN, EPD_CS);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PWR_EN, OUTPUT);

  digitalWrite(PWR_EN, HIGH);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);
  delay(100);
  digitalWrite(PIN_MOTOR, HIGH);
  delay(200);
  digitalWrite(PIN_MOTOR, LOW);

  gXdisplay.init();
  gXdisplay.setRotation(0);
  // display.invertDisplay(true);
  gXdisplay.fillScreen(GxEPD_BLACK);
  gXdisplay.drawBitmap(0, 0, scuttleshell, 200, 200, GxEPD_WHITE);
  gXdisplay.update();
  // delay(3000);

  // DeepSleep();

  esp_efuse_mac_get_default(my_mac);
#if !defined(HAS_UDP)  // in case of no Wifi: display BT mac addr, instead
  my_mac[5] += 2;
  // https://docs.espressif.com/projects/esp-idf/en/release-v3.0/api-reference/system/base_mac_address.html
#endif
#if defined(HAS_UDP) || defined(HAS_BT) || defined(HAS_BLE)
  sprintf(ssid, "%s-%s", tSSB_WIFI_SSID, to_hex(my_mac+4, 2));
  Serial.printf("This is node %s\r\n\r\n", ssid);
#endif
  io_init();

  theDmx   = new DmxClass();
}


void loopBLE()
{
#if defined(MAIN_BLEDevice_H_) && !defined(NO_BLE)
  unsigned char *cp = ble_fetch_received();
  if (cp != NULL)
    incoming(&ble_face, cp+1, *cp, 0);
#endif
}


void loop()
{
  loopBLE();
  delay(5);
}
