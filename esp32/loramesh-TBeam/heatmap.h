// heatmap.h

#include <sodium/crypto_hash_sha256.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

int hm_clock;
unsigned hm_dmx_req[7];
unsigned hm_dmx_rep[7];

unsigned long hm_next;

extern File hm_log;

void hm_save2log(char *s)
{
  hm_log.printf("%04d-%02d-%02dT%02d:%02d:%02dZ: \"%s\"\r\n",
                gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute(), gps.time.second(), s);
  hm_log.flush();
}

void hm_incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                     struct face_s *face)
{
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("incoming reply: %s\r\n", str);
  hm_save2log(str);
}

void hm_incoming_req(unsigned char *pkt, int len, unsigned char *aux,
                     struct face_s *face)
{
  // send a reply
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("incoming request: %s %dB\r\n", str, len);
  hm_save2log(str);

  char buf[100];
  memcpy(buf, hm_dmx_rep, 7);
  sprintf(buf+7, "R %02x%02x [%s]", my_mac[4], my_mac[5], str);

  if (gps.location.isValid())
    sprintf(buf+7+strlen(buf+7), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());

  io_enqueue((unsigned char*) buf, 7 + strlen(buf+7), NULL, &lora_face);
}

void hm_init()
{
  char *req = "heatmap 1.0 request";
  char *rep = "heatmap 1.0 reply";
  
  struct crypto_hash_sha256_state h;
  unsigned char out[crypto_hash_sha256_BYTES];
  crypto_hash_sha256_init(&h);
  crypto_hash_sha256_update(&h, (unsigned char*) req, strlen(req));
  crypto_hash_sha256_final(&h, out);
  memcpy(hm_dmx_req, out, DMX_LEN);

  crypto_hash_sha256_init(&h);
  crypto_hash_sha256_update(&h, (unsigned char*) rep, strlen(rep));
  crypto_hash_sha256_final(&h, out);
  memcpy(hm_dmx_rep, out, DMX_LEN);

  dmx->arm_dmx((unsigned char*) hm_dmx_req, hm_incoming_req, NULL, 0);
  dmx->arm_dmx((unsigned char*) hm_dmx_rep, hm_incoming_rep, NULL, 0);

  hm_log = MyFS.open(HEATMAP_FILENAME, FILE_APPEND);
  hm_log.printf("reboot %s\n", to_hex(my_mac,6,1));
}

void hm_tick()
{
  long now = millis();

  if (now > hm_next) {
    // send a request
    char buf[100];
    memcpy((unsigned char*)buf, hm_dmx_req, 7);
    sprintf(buf+7, "Q %02x%02x %d", my_mac[4], my_mac[5], hm_clock++);

    if (gps.location.isValid())
      sprintf(buf+7+strlen(buf+7), " gps=%.8g,%.8g,%g", gps.location.lat(),
              gps.location.lng(), gps.altitude.meters());
    
    io_enqueue((unsigned char*) buf, 7 + strlen(buf+7), NULL, &lora_face);

    hm_next = millis() + 10000 + esp_random() % 1000;
  }
}

// eof
