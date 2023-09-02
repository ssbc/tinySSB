// kiss.h

// tinySSB for ESP32
// Aug 2022 <christian.tschudin@unibas.ch>

// -------------------------------------------------------------------------------------

struct kiss_buf {
  char esc;
  unsigned char buf[256];
  short len;
};

struct kiss_buf serial_kiss, bt_kiss;

#define KISS_FEND   0xc0
#define KISS_FESC   0xdb
#define KISS_TFEND  0xdc
#define KISS_TFESC  0xdd

void kiss_write(Stream &s, unsigned char *buf, short len) {
  s.write(KISS_FEND);
  for (int i = 0; i < len; i++, buf++) {
    if (*buf == KISS_FESC) {
      s.write(KISS_FESC); s.write(KISS_TFESC);
    } else if (*buf == KISS_FEND) {
      s.write(KISS_FESC); s.write(KISS_TFEND);
    } else
      s.write(*buf);
  }
  s.write(KISS_FEND);
}

int kiss_read(Stream &s, struct kiss_buf *kp) {
  while (s.available()) {
    short c = s.read();
    Serial.println("kiss input" + String(c));
    if (c == KISS_FEND) {
      kp->esc = 0;
      short sz = 0;
      if (kp->len != 0) {
        // Serial.printf("KISS packet, %d bytes\n", kp->len);
        sz = kp->len;
        kp->len = 0;
      }
      return sz;
    }
    if (c == KISS_FESC) {
      kp->esc = 1;
    } else if (kp->esc) {
      if (c == KISS_TFESC || c == KISS_TFEND) {
        if (kp->len < sizeof(kp->buf))
          kp->buf[kp->len++] = c == KISS_TFESC ? KISS_FESC : KISS_FEND;
      }
      kp->esc = 0;
    } else if (kp->len < sizeof(kp->buf))
      kp->buf[kp->len++] = c;
  }
  return 0;
}

// eof
