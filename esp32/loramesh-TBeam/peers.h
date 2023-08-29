// peers.h

// sends "I am here beacons", permits to create a list of visible peers

#include <sodium/crypto_hash_sha256.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

int peer_clock;
unsigned peer_dmx_req[7];
unsigned peer_dmx_rep[7];

unsigned long peer_next;

extern File peer_log;

void peer_save2log(char *s)
{
#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  peer_log.printf("%04d-%02d-%02dT%02d:%02d:%02dZ: \"%s\"\r\n",
                gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute(), gps.time.second(), s);
  peer_log.flush();
#endif
}

void peer_incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                     struct face_s *face)
{
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("   P: got pong <%s> %dB rssi=%d snr=%g\r\n",
                str, len, lora_prssi, lora_psnr);

  sprintf(str+strlen(str), " / rssi=%d snr=%g", lora_prssi, lora_psnr);
  peer_save2log(str);
}

void peer_incoming_req(unsigned char *pkt, int len, unsigned char *aux,
                     struct face_s *face)
{
  // send a reply (pong)
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("   P: got ping <%s> %dB rssi=%d snr=%g\r\n",
                str, len, lora_prssi, lora_psnr);

  char buf[100];
  memcpy(buf, peer_dmx_rep, 7);
  sprintf(buf+7, "R %02x%02x t=%d rssi=%d snr=%g",
          my_mac[4], my_mac[5], peer_clock, lora_prssi, (double) lora_psnr);

#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  if (gps.location.isValid())
    sprintf(buf+7+strlen(buf+7), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());
#endif
  // append what we received, in brackets
  sprintf(buf+7+strlen(buf+7), " [%s]", str);

  delay(esp_random() % 500); // spread answers in time
  io_enqueue((unsigned char*) buf, 7 + strlen(buf+7), NULL, &lora_face);

  sprintf(str+strlen(str), " / rssi=%d snr=%g", lora_prssi, lora_psnr);
  peer_save2log(str);
}

void peer_init()
{
  char *req = "peers 1.0 request";
  char *rep = "peers 1.0 reply";
  
  struct crypto_hash_sha256_state h;
  unsigned char out[crypto_hash_sha256_BYTES];
  crypto_hash_sha256_init(&h);
  crypto_hash_sha256_update(&h, (unsigned char*) req, strlen(req));
  crypto_hash_sha256_final(&h, out);
  memcpy(peer_dmx_req, out, DMX_LEN);

  crypto_hash_sha256_init(&h);
  crypto_hash_sha256_update(&h, (unsigned char*) rep, strlen(rep));
  crypto_hash_sha256_final(&h, out);
  memcpy(peer_dmx_rep, out, DMX_LEN);

  dmx->arm_dmx((unsigned char*) peer_dmx_req, peer_incoming_req, NULL, 0);
  dmx->arm_dmx((unsigned char*) peer_dmx_rep, peer_incoming_rep, NULL, 0);

  peer_log = MyFS.open(PEERS_FILENAME, FILE_APPEND);
  peer_log.printf("\r\nrestart %s\r\n", to_hex(my_mac,6,1));
}

void peer_tick()
{
  long now = millis();

  if (now > peer_next) {
    // send a request (ping)
    char buf[100];
    memcpy((unsigned char*)buf, peer_dmx_req, 7);
    sprintf(buf+7, "Q %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock++);

#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
    if (gps.location.isValid())
      sprintf(buf+7+strlen(buf+7), " gps=%.8g,%.8g,%g", gps.location.lat(),
              gps.location.lng(), gps.altitude.meters());
#endif
    
    io_enqueue((unsigned char*) buf, 7 + strlen(buf+7), NULL, &lora_face);

    peer_next = millis() + PEERS_INTERVAL + esp_random() % 2000;
  }
}

// eof
