// io_buf.cpp

#include <Arduino.h>
#include <string.h>

#include "io_buf.h"

RingBuffer::RingBuffer(short capacity, short len)
{
  buf = (unsigned char*) malloc(capacity * (len+1));
  max_cnt = capacity;
  item_len = len;
  cnt = offs = 0;
}

bool RingBuffer::is_empty() { return cnt <= 0; }

bool RingBuffer::is_full()  { return cnt >= max_cnt; }

static unsigned char x[130];

void RingBuffer::in(unsigned char *pkt, short len)
{
  if (is_full())
    return;
  if (len > item_len)
    len = item_len;
  unsigned char *cp = (unsigned char*) buf + offs * (item_len+1);
  *cp = len;
  memcpy(cp+1, pkt, len);
  offs = (offs + 1) % max_cnt;
  cnt++;
}

unsigned char* RingBuffer::out()
{
  if (is_empty())
    return NULL;
  unsigned char *cp = (unsigned char*) buf +
                 ((offs + max_cnt - cnt) % max_cnt) * (item_len+1);
  cnt--;
  return cp;
}

// eof
