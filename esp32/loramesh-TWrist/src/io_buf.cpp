// io_buf.cpp

#include <string.h>

#include "io_buf.h"

RingBuffer::RingBuffer(short capacity, short len)
{
  buf = (unsigned char*) malloc(capacity * (item_len+1));
  max_cnt = capacity;
  item_len = len;
  cnt = offs = 0;
}

bool RingBuffer::is_empty() { return cnt == 0; }

bool RingBuffer::is_full()  { return cnt >= max_cnt; }

void RingBuffer::in(unsigned char *pkt, short len)
{
  if (len > item_len)
    len = item_len;
  unsigned char *cp = (unsigned char*) buf + offs * (item_len+1);
  *cp = len;
  memcpy(cp+1, pkt, len);
  offs = (offs + 1) % max_cnt;
  cnt++;
}

short RingBuffer::out(unsigned char *dst)
{
  unsigned char *cp = (unsigned char*) buf +
                 ((offs + max_cnt - cnt) % max_cnt) * (item_len+1);
  short pkt_len = *cp;
  memcpy(dst, cp+1, pkt_len);
  cnt--;
  return pkt_len;
}

// eof
