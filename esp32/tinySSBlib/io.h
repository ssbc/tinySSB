// io.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

#ifndef _INCLUDE_IO_H
#define _INCLUDE_IO_H

#include "io_buf.h"

#define IO_MAX_QUEUE_LEN          5
#define LORA_INTERPACKET_TIME  1350  // millis
#define UDP_INTERPACKET_TIME    100  // millis

struct face_s {
  char *name;
  bool has_crc;
  unsigned long next_send;
  unsigned int next_delta;
  RingBuffer *in_buf;
  void (*send)(unsigned char *buf, short len);
  void (*loop)(); // check for incoming packets
};

void io_init();
void io_loop(); // check for received packets
void io_proc(); // process received packets

void io_send(unsigned char *buf, short len, struct face_s *f=NULL);

#endif

// eof
