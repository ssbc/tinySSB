// io.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

/*
   tinySSB packet format:
   dmx(7B) + more_data(var) + CRC32(4B)
*/

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "io.h"

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
#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)
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
#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)
  &ble_face
#endif
};

int lora_pkt_cnt; // for counting in/outcoming packets, per NODE round
float lora_pps;   // packet-per-seconds, gliding average

int lora_sent_pkts = 0; // absolute counter
int lora_rcvd_pkts = 0; // absolute counter


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

uint32_t crc32_ieee(unsigned char *pkt, int len) { // Ethernet/ZIP polynomial
  uint32_t crc = 0xffffffffu;
  while (len-- > 0) {
    crc ^= *pkt++;
    for (int i = 0; i < 8; i++)
      crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
  }
  return htonl(crc ^ 0xffffffffu);
}

// ---------------------------------------------------------------------------

#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)

BLECharacteristic *RXChar = nullptr; // receive
BLECharacteristic *TXChar = nullptr; // transmit (notify)
BLECharacteristic *STChar = nullptr; // statistics
int bleDeviceConnected = 0;
char txString[128] = {0};

typedef unsigned char tssb_pkt_t[1+127];
tssb_pkt_t ble_ring_buf[BLE_RING_BUF_SIZE];
int ble_ring_buf_len = 0;
int ble_ring_buf_cur = 0;

unsigned char* ble_fetch_received() // first byte has length, up to 127B
{
  unsigned char *cp;
  if (ble_ring_buf_len == 0)
    return NULL;
  cp = (unsigned char*) (ble_ring_buf + ble_ring_buf_cur);
  // noInterrupts();
  ble_ring_buf_cur = (ble_ring_buf_cur + 1) % BLE_RING_BUF_SIZE;
  ble_ring_buf_len--;
  // interrupts();
  return cp;
}

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
  void onWrite(BLECharacteristic *pChar) {
    uint16_t len = pChar->getValue().length();
    // Serial.println("RXCallback " + String(len) + " bytes");
    if (len > 0 && len <= 127 && ble_ring_buf_len < BLE_RING_BUF_SIZE) {
      // no CRC check, as none is sent for BLE
      int ndx = (ble_ring_buf_cur + ble_ring_buf_len) % BLE_RING_BUF_SIZE;
      unsigned char *pos = (unsigned char*) (ble_ring_buf + ndx);
      *pos = len;
      memcpy(pos+1, pChar->getData(), len);
      // noInterrupts();
      ble_ring_buf_len++;
      // interrupts();
    }
  }
};

extern char ssid[];

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

#endif // BLE


// --------------------------------------------------------------------------------

int lora_send_ok;

#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  volatile bool lora_fetching = false;
  volatile bool lora_transmitting = false;
  volatile bool new_lora_pkt = false;

  void newLoraPacket_cb(void)
  {
    // Serial.println("newLoraPkt");
    if (lora_fetching || lora_transmitting)
      return;
    new_lora_pkt = true;
  }
#endif


#if defined(HAS_LORA)

void lora_send(unsigned char *buf, short len)
{
#if defined(USE_RADIO_LIB)

  unsigned char *data = (unsigned char*) malloc(len+4);
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
#endif

#if defined(HAS_UDP)
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

#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)

void ble_send(unsigned char *buf, short len)
{
  if (bleDeviceConnected == 0) return;
  // no CRC added, we rely on BLE's CRC
  TXChar->setValue(buf, len);
  TXChar->notify();
  Serial.printf("b< %3dB %s..\r\n", len, to_hex(buf,8,0));
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

#if defined(HAS_BT)
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
#endif

// --------------------------------------------------------------------------------

void io_init()
{
#if defined(HAS_LORA)
  lora_face.name = (char*) "lora";
  lora_face.next_delta = LORA_INTERPACKET_TIME;
  lora_face.send = lora_send;
#endif
#if defined(HAS_UDP)
  udp_face.name = (char*) "udp";
  udp_face.next_delta = UDP_INTERPACKET_TIME;
  udp_face.send = udp_send;
#endif
#if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)
  ble_init();
  ble_face.name = (char*) "ble";
  ble_face.next_delta = UDP_INTERPACKET_TIME;
  ble_face.send = ble_send;
#endif
#if defined(HAS_BT)
  bt_face.name = (char*) "bt";
  bt_face.next_delta = UDP_INTERPACKET_TIME;
  bt_face.send = bt_send;
#endif
}

void io_send(unsigned char *buf, short len, struct face_s *f)
{
  // Serial.printf("io_send %d bytes\r\n", len);
  for (int i = 0; i < NR_OF_FACES; i++) {
    if (faces[i]->send == NULL)
      continue;
    if (f == NULL || f == faces[i])
      faces[i]->send(buf, len);
  }
}

#ifdef NOT_USED
void io_enqueue(unsigned char *pkt, int len, unsigned char *dmx, struct face_s *f)
{
  // Serial.printf("   enqueue %dB %s..\r\n", len, to_hex(dmx?dmx:pkt,7));
  for (int i = 0; i < NR_OF_FACES; i++) {
    if (faces[i]->send == NULL)
      continue;
    if (f == NULL || f == faces[i]) {
      if (faces[i]->queue_len >= IO_MAX_QUEUE_LEN) {
        Serial.printf("   IO: outgoing queue full for %s %s..\r\n",
                      faces[i]->name, to_hex(dmx?dmx:pkt,7,0));
        continue;
      }
      int sz = len + (dmx ? DMX_LEN : 0);
      unsigned char *buf = (unsigned char*) malloc(1+sz);
      buf[0] = sz;
      if (dmx) {
        memcpy(buf+1, dmx, DMX_LEN);
        memcpy(buf+1+DMX_LEN, pkt, len);
      } else
        memcpy(buf+1, pkt, len);
      /*
      io_send(buf+1, sz, faces[i]);
      free(buf);
      continue;
      */
     
      // only insert if packet content is not already enqueued:
      int k;
      for (k = 0; k < faces[i]->queue_len; k++)
        if (!memcmp(faces[i]->queue[(faces[i]->offs + k)%IO_MAX_QUEUE_LEN], buf, 1+sz))
          break;
      if (k == faces[i]->queue_len) {
        Serial.printf("   enqueue %dB %s.. on face %s\r\n", *buf, to_hex(buf+1,7), faces[i]->name);
        faces[i]->queue[(faces[i]->offs + faces[i]->queue_len) % IO_MAX_QUEUE_LEN] = buf;
        faces[i]->queue_len++;
      } else
        free(buf);
    }
  }
}

/*
void io_dequeue() // enforces interpacket time
{
  unsigned long now = millis();
  for (int i = 0; i < NR_OF_FACES; i++) {
    struct face_s *f = faces[i];
    if (f->send == NULL || f->queue_len <= 0 || now < f->next_send) // FIXME: handle wraparound
      continue;
    // Serial.printf("   dequeue i=%d, len=%d\r\n", i, f->queue_len);
    f->next_send = now + f->next_delta + esp_random() % (f->next_delta/10);
    unsigned char *buf = f->queue[f->offs];
    // io_send(buf+1, *buf, f);
    f->send(buf+1, *buf);
    free(buf);
    f->offs = (f->offs + 1) % IO_MAX_QUEUE_LEN;
    f->queue_len--;
  }
}
*/

#endif

int crc_check(unsigned char *pkt, int len) // returns 0 if OK
{
  uint32_t crc = crc32_ieee(pkt, len-sizeof(crc));
  return memcmp(pkt+len-sizeof(crc), (void*)&crc, sizeof(crc));
}


// ---------------------------------------------------------------------------
// int lora_new_cnt = 0;

/*
void newLoRaPkt(int sz) {
  int packetSize = sz;
  // LoRa.parsePacket();
  // if (packetSize != sz)
  //   Serial.printf("lora <> %d %d\r\n", packetSize, sz);
  // if (packetSize == 0 ||
  
  if (lora_buf_cnt >= LORA_BUF_CNT)
    return;
  unsigned char *pkt = (unsigned char*) lora_buf + lora_buf_offs * (LORA_MAX_LEN+1);
  int cnt;
  if (packetSize > LORA_MAX_LEN)
    packetSize = LORA_MAX_LEN;
  *pkt++ = packetSize;
  for (cnt = 0; cnt < packetSize; cnt++)
    *pkt++ = LoRa.read();
  lora_buf_offs = (lora_buf_offs + 1) % LORA_BUF_CNT;
  lora_buf_cnt++;
}
*/

// ---------------------------------------------------------------------------

class RingBuffer { // FIFO
#define LORA_BUF_CNT 4
#define LORA_MAX_LEN 127

public:
  bool is_empty();
  bool is_full();
  void in(unsigned char *pkt, short len);
  short out(unsigned char *dst);
  volatile unsigned char buf[LORA_BUF_CNT * (LORA_MAX_LEN+1)];
  volatile short cnt, offs;
};

bool RingBuffer::is_empty() { return cnt == 0; }
bool RingBuffer::is_full()  { return cnt >= LORA_BUF_CNT; }
void RingBuffer::in(unsigned char *pkt, short len)
{
  if (len > LORA_MAX_LEN)
    len = LORA_MAX_LEN;
  unsigned char *cp = (unsigned char*) buf + offs * (LORA_MAX_LEN+1);
  *cp = len;
  memcpy(cp+1, pkt, len);
  offs = (offs + 1) % LORA_BUF_CNT;
  cnt++;
}
short RingBuffer::out(unsigned char *dst)
{
  unsigned char *cp = (unsigned char*) buf +
                 ((offs + LORA_BUF_CNT - cnt) % LORA_BUF_CNT) * (LORA_MAX_LEN+1);
  short pkt_len = *cp;
  memcpy(dst, cp+1, pkt_len);
  cnt--;
  return pkt_len;
}

RingBuffer lora_buf;

// ---------------------------------------------------------------------------

#if defined(HAS_LORA)
void lora_poll()
{
#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  lora_fetching = true;
  if (new_lora_pkt) {
    // Serial.println("   lora_poll: new pkt");
    radio.standby();
    new_lora_pkt = false;

    while (-1) {
      unsigned char buf[LORA_MAX_LEN];
      size_t len = radio.getPacketLength();
      if (len <= 0)
        break;
      // Serial.printf("   lora_poll: len=%d\r\n", len);
      lora_rcvd_pkts++;
      lora_pkt_cnt++;
      int rc = radio.readData(buf, len);
      if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("  readData returned %d\r\n", rc);
        break;
      }
      if (lora_buf.is_full()) {
        Serial.println("   too many LoRa packets, dropped one");
      } else
        lora_buf.in(buf, len);
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

int lora_get_pkt(unsigned char *dst)
{
  if (lora_buf.is_empty())
    return 0;
  return lora_buf.out(dst);
}
#endif

// eof
