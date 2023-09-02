// peers.cpp

#include "tinySSBlib.h"


void peer_incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                       struct face_s *face)
{
  thePeers->incoming_rep(pkt, len, aux, face);
}


void peer_incoming_req(unsigned char *pkt, int len, unsigned char *aux,
                       struct face_s *face)
{
  thePeers->incoming_req(pkt, len, aux, face);
}


PeersClass::PeersClass()
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

  theDmx->arm_dmx(peer_dmx_req, peer_incoming_req, NULL, 0);
  theDmx->arm_dmx(peer_dmx_rep, peer_incoming_rep, NULL, 0);

  Serial.printf("   DMX for PEEQ is %s\r\n", to_hex(peer_dmx_req, 7, 0));
  Serial.printf("   DMX for PEER is %s\r\n", to_hex(peer_dmx_rep, 7, 0));

  // peer_log = MyFS.open(PEERS_FILENAME, FILE_APPEND);
  // peer_log.printf("\r\nrestart %s\r\n", to_hex(my_mac,6,1));

  peer_clock = 0;
}


void PeersClass::probe_for_peers_beacon(unsigned char **pkt,
                                        unsigned short *len,
                                        unsigned short *reprobe_in_millis)
{
  Serial.println("   prepare PEER beacon");

  // send a request (ping)
  char buf[100];
  sprintf(buf, "Q %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock++);

#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  if (gps.location.isValid())
    sprintf(buf+strlen(buf), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());
#endif

  theSched->schedule_asap((unsigned char*) buf, strlen(buf),
                          peer_dmx_req);

  *reprobe_in_millis = PEERS_INTERVAL + esp_random() % 2000;
}


void PeersClass::incoming_req(unsigned char *pkt, int len, unsigned char *aux,
                              struct face_s *face)
{
  // send a reply (pong)
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("   =P.ping <%s> %dB", str, len);
#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  Serial.print(" rssi=%d snr=%g",
                str, len, (int) radio.getRSSI(), radio.getSNR());
#endif
  Serial.printf("\r\n");

  char buf[100];
  sprintf(buf, "R %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock);

#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  sprintf(buf+strlen(buf), " rssi=%d snr=%g"
                                   (int) radio.getRSSI(), radio.getSNR());
#endif
  
#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  if (gps.location.isValid())
    sprintf(buf+strlen(buf), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());
#endif
  // append what we received, in brackets
  sprintf(buf+strlen(buf), " [%s]", str);

  theSched->schedule_asap((unsigned char*) buf, strlen(buf),
                          peer_dmx_rep, face);

  // sprintf(str+strlen(str), " / rssi=%d snr=%g",
  //        (int) radio.getRSSI(), radio.getSNR());
  // peer_save2log(str);

  // str[6] = '\0';
  // theStatus->heard_peer(str+2, lora_prssi, lora_psnr);
}


void PeersClass::incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                              struct face_s *face)
{
  char str[100];
  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';

  Serial.printf("   =P.pong <%s> %dB", str, len);
#if defined(HAS_LORA) && defined(USE_RADIO_LIB)
  Serial.print(" rssi=%d snr=%g", (int) radio.getRSSI(), radio.getSNR());
#endif
  Serial.printf("\r\n");

  // sprintf(str+strlen(str), " / rssi=%d snr=%g",
  //        (int) radio.getRSSI(), radio.getSNR());
  // peer_save2log(str);

  // str[6] = '\0';
  // theStatus->heard_peer(str+2, lora_prssi, lora_psnr);
}



/*

void PeersClass::save2log(char *s)
{
#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
  if (gps.date.isValid())
    peer_log.printf("%04d-%02d-%02dT%02d:%02d:%02dZ: \"%s\"\r\n",
                gps.date.year(), gps.date.month(), gps.date.day(),
                gps.time.hour(), gps.time.minute(), gps.time.second(), s);
  else
#else
  peer_log.printf("%d: \"%s\"\r\n", millis(), s);
#endif
  peer_log.flush();
}
*/



// eof
