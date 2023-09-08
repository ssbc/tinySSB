// io_buf.h

#ifndef _INCLUDE_RING_BUFFER
#define _INCLUDE_RING_BUFFER

#include "tinySSBlib.h"


class RingBuffer { // FIFO

public:
  RingBuffer(short capacity, short item_len);
  bool is_empty();
  bool is_full();
  void in(unsigned char *pkt, short len);
  // short out(unsigned char *dst);
  unsigned char* out();
  int get_len() { return cnt; };

private:
  unsigned char *buf; // [LORA_BUF_CNT * (LORA_MAX_LEN+1)];
  volatile short cnt, offs;
  short max_cnt, item_len;
};

#endif
