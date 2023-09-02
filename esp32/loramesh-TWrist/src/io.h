// io.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

#if !defined(_INCLUDE_IO_H)
#define _INCLUDE_IO_H

#define IO_MAX_QUEUE_LEN          5
#define LORA_INTERPACKET_TIME  1350  // millis
#define UDP_INTERPACKET_TIME    100  // millis

struct face_s {
  char *name;
  unsigned long next_send;
  unsigned int next_delta;
  unsigned char *queue[IO_MAX_QUEUE_LEN]; // ring buffer of |len|...pkt...|
  int queue_len;
  int offs = 0;
  void (*send)(unsigned char *buf, short len);
};

extern struct face_s lora_face;
extern struct face_s udp_face;
extern struct face_s bt_face;
// #if defined(MAIN_BLEDevice_H_) && defined(HAS_BLE)
  extern struct face_s ble_face;
// #endif

extern int bleDeviceConnected;
extern int lora_pkt_cnt; // for counting in/outcoming packets, per NODE round
extern float lora_pps;   // packet-per-seconds, gliding average

extern int lora_sent_pkts; // absolute counter
extern int lora_rcvd_pkts; // absolute counter

int incoming(struct face_s *f, unsigned char *pkt, int len, int has_crc);
uint32_t crc32_ieee(unsigned char *pkt, int len); // Ethernet/ZIP polynomial
unsigned char* ble_fetch_received(); // first byte has length, up to 127B
void ble_init();
void ble_send_stats(unsigned char *str, short len);
void io_init();
void io_loop(); // check for receoved packets
void io_send(unsigned char *buf, short len, struct face_s *f=NULL);
/*
void io_enqueue(unsigned char *pkt, int len,
                unsigned char *dmx=NULL, struct face_s *f=NULL);
void io_dequeue(); // enforces interpacket time
*/
int crc_check(unsigned char *pkt, int len); // returns 0 if OK

// int fishForNewLoRaPkt();
void lora_poll();
int lora_get_pkt(unsigned char *dst);

#endif

// eof
