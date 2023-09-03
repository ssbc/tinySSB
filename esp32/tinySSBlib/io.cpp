// io.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

/*
   tinySSB packet format:
   dmx(7B) + more_data(var) + CRC32(4B)

   BLE has no CRC value
*/

#include "tinySSBlib.h"
#include "io_buf.h"

#define MAX_PKT_LEN 127

#define NR_OF_FACES               (sizeof(faces) / sizeof(void*))

#if defined(HAS_LORA)
  struct face_s lora_face;
#endif
#if defined(HAS_UDP)
  struct face_s udp_face;
#endif
#if defined(HAS_BT)
  struct face_s bt_face;
#endif
#if defined(HAS_BLE)
  struct face_s ble_face;
#endif

struct face_s *faces[] = {
#if defined(HAS_LORA)
  &lora_face,
#endif
#if defined(HAS_UDP)
  &udp_face,
#endif
#if defined(HAS_BT)
  &bt_face,
#endif
#if defined(HAS_BLE)
  &ble_face
#endif
};


// ---------------------------------------------------------------------------

uint32_t crc32_ieee(unsigned char *pkt, int len) { // Ethernet/ZIP polynomial
  uint32_t crc = 0xffffffffu;
  while (len-- > 0) {
    crc ^= *pkt++;
    for (int i = 0; i < 8; i++)
      crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
  }
  return htonl(crc ^ 0xffffffffu);
}

int crc_check(unsigned char *pkt, int len) // returns 0 if OK
{
  uint32_t crc = crc32_ieee(pkt, len-sizeof(crc));
  return memcmp(pkt+len-sizeof(crc), (void*)&crc, sizeof(crc));
}

// ---------------------------------------------------------------------------

int incoming(struct face_s *f, unsigned char *pkt, int len, int has_crc)
{
  unsigned char h[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(h, pkt, len);
  Serial.printf("%c> %dB %s..", f->name[0], len, to_hex(pkt, DMX_LEN));
  if (has_crc)
    Serial.printf("%s ", to_hex(pkt + len-6-sizeof(uint32_t), 6));
  else
    Serial.printf("%s ", to_hex(pkt + len-6, 6));
  Serial.printf("h=%s\r\n", to_hex(h, HASH_LEN));
  
  if (len <= (DMX_LEN + sizeof(uint32_t))) {
    Serial.printf("   =short packet\r\n");
    // lora_bad_crc++;
    return -1;
  }
  if (has_crc && crc_check(pkt,len)) {
    Serial.printf("   =bad CRC\r\n");
    // lora_bad_crc++;
    return -1;
  }
  if (has_crc) {
    // Serial.println("CRC OK");
    len -= sizeof(uint32_t);
  }
  // Serial.printf("<  incoming packet, %d bytes\r\n", len);

  if (!theDmx->on_rx(pkt, len, h, f))
    return 0;
  Serial.println(String("   unknown DMX ") + to_hex(pkt, DMX_LEN, 0));
  return -1;
}

// ---------------------------------------------------------------------------

#if defined(HAS_BLE)

# include <BLEDevice.h>
# include <BLEServer.h>
# include <BLEUtils.h>
# include <BLE2902.h>
# include "esp_gatt_common_api.h"

BLECharacteristic *RXChar = nullptr; // receive
BLECharacteristic *TXChar = nullptr; // transmit (notify)
BLECharacteristic *STChar = nullptr; // statistics
volatile int bleDeviceConnected = 0;
char txString[128] = {0};

class UARTServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bleDeviceConnected += 1;
      Serial.println("** BLE device connected");
      // stop advertising when a peer is connected (we can only serve one client)
      if (bleDeviceConnected == 3) { pServer->getAdvertising()->stop(); }
      else { pServer->getAdvertising()->start(); }
    };
    void onDisconnect(BLEServer* pServer) {
      bleDeviceConnected -= 1;
      Serial.println("** BLE device disconnected");
      // resume advertising when peer disconnects
      pServer->getAdvertising()->start();
    }
};


class RXCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar,
               esp_ble_gatts_cb_param_t* param) {
    // Serial.printf("RXCallback %dB\r\n", param->write.len);
    if (param->write.len > 0)
      ble_face.in_buf->in(param->write.value, param->write.len);
  }
};


void ble_init()
{
  Serial.println("BLE init");

  // Create the BLE Device
  BLEDevice::init(ssid); // "tinySSB virtual LoRa pub");
  BLEDevice::setMTU(128);
  // Create the BLE Server
  BLEServer *UARTServer = BLEDevice::createServer();
  // UARTServer->setMTU(128);
  UARTServer->setCallbacks(new UARTServerCallbacks());
  // Create the BLE Service
  BLEService *UARTService = UARTServer->createService(BLE_SERVICE_UUID);

  // Create our BLE Characteristics
  TXChar = UARTService->createCharacteristic(BLE_CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  TXChar->addDescriptor(new BLE2902());
  TXChar->setNotifyProperty(true);
  TXChar->setReadProperty(true);

  STChar = UARTService->createCharacteristic(BLE_CHARACTERISTIC_UUID_ST, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  STChar->addDescriptor(new BLE2902());
  STChar->setNotifyProperty(true);
  STChar->setReadProperty(true);

  RXChar = UARTService->createCharacteristic(BLE_CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  RXChar->setCallbacks(new RXCallbacks());

  // Start the service
  UARTService->start();
  // Start advertising
  UARTServer->getAdvertising()->start();
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(128); // 23);
  if (local_mtu_ret) {
    Serial.println("set local MTU failed, error code = " + String(local_mtu_ret));
  }

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}


void ble_send(unsigned char *buf, short len)
{
  // Serial.printf("BLE send %p %d\r\n", buf, len);
  if (bleDeviceConnected == 0)
    return;
  // no CRC added, we rely on BLE's CRC
  TXChar->setValue(buf, len);
  TXChar->notify();
  Serial.printf("b< %dB %s..\r\n", len, to_hex(buf,8,0));
}


void ble_send_stats(unsigned char *str, short len)
{
  if (bleDeviceConnected == 0) return;
  // no CRC added, we rely on BLE's CRC
  STChar->setValue(str, len);
  STChar->notify();
  // Serial.printf("   BLE  sent %3dB stat <%s>\r\n", len, str);
}

#endif // BLE


// ---------------------------------------------------------------------------

#if defined(HAS_LORA)

#if defined(USE_RADIO_LIB)
# if defined(TINYSSB_BOARD_TDECK)
   SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN,
                             RADIO_RST_PIN, RADIO_BUSY_PIN);
# endif
#else
# include <LoRa.h>
#endif

int lora_send_ok;

int lora_pkt_cnt; // for counting in/outcoming packets, per NODE round
float lora_pps;   // packet-per-seconds, gliding average

int lora_sent_pkts = 0; // absolute counter
int lora_rcvd_pkts = 0; // absolute counter

#if defined(USE_RADIO_LIB)
  volatile bool lora_fetching = false;
  volatile bool lora_transmitting = false;
  volatile bool new_lora_pkt = false;

  void newLoraPacket_cb(void)
  {
    // Serial.println("newLoraPkt");
    /*
    if (lora_transmitting) {
      radio.finishTransmit();
      lora_transmitting = false;
      return;
    }
    */
    if (lora_fetching || lora_transmitting)
      return;
    new_lora_pkt = true;
  }
#endif



void lora_send(unsigned char *buf, short len)
{
  // Serial.printf("lora send %p %d\r\n", buf, len);

#if defined(USE_RADIO_LIB)

  unsigned char *data = (unsigned char*) malloc(len+sizeof(uint32_t));
  memcpy(data, buf, len);
  uint32_t crc = crc32_ieee(buf, len);
  memcpy(data+len, &crc, sizeof(crc));

  lora_transmitting = true;
  lora_send_ok = radio.transmit(data, len + sizeof(crc), 0) == RADIOLIB_ERR_NONE;
  free(data);

  if (lora_send_ok) {
    Serial.printf("l< %dB %s..",
                  len + sizeof(crc), to_hex(buf,7,0));
    Serial.printf("%s @%d\r\n", to_hex(buf + len - 6, 6, 0), millis());
  } else {
    Serial.printf("   LoRa send fail %dB %s..", len, to_hex(buf,7,0));
    Serial.printf("%s @%d\r\n", to_hex(buf + len - 6, 6, 0), millis());
  }
  lora_pkt_cnt++;
  lora_sent_pkts++;

  lora_transmitting = false;
  radio.startReceive();

#else

  if (LoRa.beginPacket()) {
    lora_pkt_cnt++;
    lora_sent_pkts++;
    uint32_t crc = crc32_ieee(buf, len);
    LoRa.write(buf, len);
    LoRa.write((unsigned char*) &crc, sizeof(crc));
    if (LoRa.endPacket()) {
      Serial.printf("   LoRa sent %3dB %s..",
                  len + sizeof(crc), to_hex(buf,7,0)); // + to_hex(buf + len - 6, 6));
      Serial.printf("%s @%d\r\n", to_hex(buf + len - 6, 6, 0), millis());
      lora_send_ok = 1;
    } else
      Serial.printf("   LoRa fail %-3dB %s.. @%d\r\n", len,
                    to_hex(buf + len - 6, 6, 0), millis());
      lora_send_ok = 0;
  } else
    Serial.println("   LoRa send failed");

#endif // USE_RADIO_LIB
}


void lora_loop()
{
#if defined(USE_RADIO_LIB)
  lora_fetching = true;
  if (new_lora_pkt) {
    // Serial.println("   lora_poll: new pkt");
    radio.standby();
    new_lora_pkt = false;

    while (-1) {
      unsigned char buf[MAX_PKT_LEN];
      size_t len = radio.getPacketLength();
      if (len <= 0)
        break;
      if (len > MAX_PKT_LEN)
        len = MAX_PKT_LEN;
      // Serial.printf("   lora_poll: len=%d\r\n", len);
      lora_rcvd_pkts++;
      lora_pkt_cnt++;
      int rc = radio.readData(buf, len);
      if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("  readData returned %d\r\n", rc);
        break;
      }
      if (lora_face.in_buf->is_full()) {
        Serial.println("   too many LoRa packets, dropped one");
      } else
        lora_face.in_buf->in(buf, len);
      break;
    }
    radio.startReceive();
  }

  lora_fetching = false;
#else // LoRa lib
    while (-1) {
    int sz = LoRa.parsePacket();
    if (sz <= 0)
      return; // lora_buf.cnt;
    lora_pkt_cnt++;
    lora_rcvd_pkts++;
    if (lora_buf.cnt >= LORA_BUF_CNT) {
      Serial.printf("   ohh %d, rcvd too many LoRa pkts, cnt=%d\r\n",
                    sz, lora_buf.cnt);
      while (sz-- > 0)
        LoRa.read();
      continue;
    }
    if (sz > LORA_MAX_LEN)
      sz = LORA_MAX_LEN;
    unsigned char *pkt = (unsigned char*) lora_buf.buf + lora_buf.offs * (LORA_MAX_LEN+1);
    unsigned char *ptr = pkt;
    *ptr++ = sz;
    while (sz-- > 0)
      *ptr++ = LoRa.read();
    lora_buf.offs = (lora_buf.offs + 1) % LORA_BUF_CNT;
    lora_buf.cnt++;
    Serial.printf("   rcvd %dB on lora, %s.., now %d pkts in buf\r\n", *pkt, to_hex(pkt+1, 7), lora_buf.cnt);
  }
  /*
  while (-1) {
    int sz = LoRa.parsePacket();
    if (sz <= 0)
      return lora_buf.cnt;
    lora_pkt_cnt++;
    lora_rcvd_pkts++;
    if (lora_buf.cnt >= LORA_BUF_CNT) {
      Serial.printf("   ohh %d, rcvd too many LoRa pkts, cnt=%d\r\n",
                    sz, lora_buf.cnt);
      while (sz-- > 0)
        LoRa.read();
      continue;
    }
    if (sz > LORA_MAX_LEN)
      sz = LORA_MAX_LEN;
    unsigned char *pkt = (unsigned char*) lora_buf.buf + lora_buf.offs * (LORA_MAX_LEN+1);
    unsigned char *ptr = pkt;
    *ptr++ = sz;
    while (sz-- > 0)
      *ptr++ = LoRa.read();
    lora_buf.offs = (lora_buf.offs + 1) % LORA_BUF_CNT;
    lora_buf.cnt++;
    Serial.printf("   rcvd %dB on lora, %s.., now %d pkts in buf\r\n", *pkt, to_hex(pkt+1, 7), lora_buf.cnt);
  }
  */
#endif
}

#endif // HAS_LORA

// ---------------------------------------------------------------------------

#if defined(HAS_UDP)

# include <WiFi.h>
# include <WiFiAP.h>

void udp_send(unsigned char *buf, short len)
{
#if !defined(NO_WIFI)
  if (udp.beginMulticastPacket()) {
    uint32_t crc = crc32_ieee(buf, len);
    udp.write(buf, len);
    udp.write((unsigned char*) &crc, sizeof(crc));
    udp.endPacket();
    Serial.println("   UDP  sent " + String(len + sizeof(crc), DEC) + "B: "
                   + to_hex(buf,8,0) + ".." + to_hex(buf + len - 6, 6, 0));
  } else
    Serial.println("udp send failed");
  /*
  if (udp_sock >= 0 && udp_addr_len > 0) {
    if (lwip_sendto(udp_sock, buf, len, 0,
                  (sockaddr*)&udp_addr, udp_addr_len) < 0)
        // err_cnt += 1;
    }
  */
#endif
}
#endif // HAS_UDP

// ---------------------------------------------------------------------------

#if defined(HAS_BT)

#include <BluetoothSerial.h>
BluetoothSerial BT;

extern void kiss_write(Stream &s, unsigned char *buf, short len);

void bt_send(unsigned char *buf, short len)
{
  if (BT.connected()) {
    uint32_t crc = crc32_ieee(buf, len);
    unsigned char *buf2 = (unsigned char*) malloc(len + sizeof(crc));
    memcpy(buf2, buf, len);
    memcpy(buf2+len, &crc, sizeof(crc));
    kiss_write(BT, buf2, len+sizeof(crc));
    Serial.println("   BT   sent " + String(len + sizeof(crc)) + "B: "
                   + to_hex(buf2,8) + ".." + to_hex(buf2 + len + sizeof(crc) - 6, 6, 0));

  } // else
    // Serial.println("BT not connected");
}

#endif // HAS_B

// ---------------------------------------------------------------------------

void io_init()
{
#if defined(HAS_LORA)
  lora_face.name = (char*) "lora";
  lora_face.has_crc = true;
  lora_face.next_delta = LORA_INTERPACKET_TIME;
  lora_face.in_buf = new RingBuffer(4, MAX_PKT_LEN);
  lora_face.send = lora_send;
  lora_face.loop = lora_loop;
#endif
#if defined(HAS_UDP)
  udp_face.name = (char*) "udp";
  udp_face.has_crc = false;
  udp_face.next_delta = UDP_INTERPACKET_TIME;
  udp_face.send = udp_send;
#endif
#if defined(HAS_BLE)
  ble_init();
  ble_face.name = (char*) "ble";
  ble_face.has_crc = false;
  ble_face.next_delta = UDP_INTERPACKET_TIME;
  ble_face.in_buf = new RingBuffer(BLE_RING_BUF_SIZE, MAX_PKT_LEN);
  ble_face.send = ble_send;
#endif
#if defined(HAS_BT)
  bt_face.name = (char*) "BT";
  bt_face.has_crc = false;
  bt_face.next_delta = UDP_INTERPACKET_TIME;
  bt_face.send = bt_send;
#endif
}

void io_send(unsigned char *buf, short len, struct face_s *f)
{
  // Serial.printf("io_send %d bytes %p\r\n", len, f);
  for (int i = 0; i < NR_OF_FACES; i++) {
    if (faces[i]->send == NULL)
      continue;
    if (f == NULL || f == faces[i])
      faces[i]->send(buf, len);
  }
}

// ---------------------------------------------------------------------------

void io_loop()
{
  for (int i = 0; i < NR_OF_FACES; i++)
    if (faces[i]->loop)
      faces[i]->loop();
}

void io_proc()
{
  for (int i = 0; i < NR_OF_FACES; i++) {
    struct face_s *f = faces[i];
    unsigned char *cp = f->in_buf->out();
    if (cp != NULL)
      incoming(f, cp+1, *cp, f->has_crc);
  }
}

// eof
