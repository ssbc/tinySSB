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

  peer_clock = 0;

  memset(heard_peers, 0, sizeof(heard_peers));
  avg_peer_density = 0;
}


void PeersClass::probe_for_peers_beacon(unsigned char **pkt,
                                        unsigned short *len,
                                        unsigned short *reprobe_in_millis)
{
  Serial.println("   prepare PEER beacon");

  long now = millis();
  int cnt = 0;
  for (int i = 0; i < MAX_HEARD_PEERS; i++) {
    struct peer_s *p = heard_peers + i;
    if (p->id[0] == '\0')
      break;
    long since = (now - p->when) / 1000;
    if (since >= 180) { // remove stale peers after 3 min
      memmove(heard_peers + i, heard_peers + i + 1,
              (MAX_HEARD_PEERS-i-1) * sizeof(struct peer_s));
      heard_peers[MAX_HEARD_PEERS-1].id[0] = '\0';
    } else
      cnt++;
  }
  avg_peer_density = 0.3 * avg_peer_density + 0.7 * cnt;

  // send a request (ping)
  char buf[100];
  sprintf(buf, "Q %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock++);

#ifdef HAS_GPS
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

  memcpy((unsigned char*) str, pkt+7, len-7); // extract payload
  str[len-7] = '\0';
  Serial.printf("   =P.ping <%s> %dB", str, len);

  char buf[200];
  sprintf(buf, "R %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock);
  
#ifdef HAS_LORA
  int rssi;
  double snr;
# ifdef USE_RADIO_LIB
  rssi = (int) radio.getRSSI(), snr = radio.getSNR();
# else
  rssi = LoRa.packetRssi(), snr = LoRa.packetSnr();
# endif
  Serial.printf(" rssi=%ddBm snr=%.1f", rssi, snr);
  sprintf(buf+strlen(buf), " rssi=%ddBm snr=%.1f", rssi, snr);
#endif
  Serial.printf("\r\n");
  
#ifdef HAS_GPS
  if (gps.location.isValid())
    sprintf(buf+strlen(buf), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());
#endif
  // append what we received, in brackets
  sprintf(buf+strlen(buf), " [%s]", str);

  if (strlen(buf) >= 113)
    buf[113] = '\0';
  theSched->schedule_asap((unsigned char*) buf, strlen(buf),
                          peer_dmx_rep, face);
  // we log the received request via our reply 'R' that we send back
  peers_log_wr(buf);

#ifdef HAS_LORA
  str[6] = '\0';
  _update_peer(str+2, rssi, snr);
#endif
}


void PeersClass::incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                              struct face_s *face)
{
  char str[100];

  memcpy((unsigned char*) str, pkt+7, len-7);
  str[len-7] = '\0';
  Serial.printf("   =P.pong <%s> %dB", str, len);

  char buf[250];
  sprintf(buf, "A %02x%02x t=%d", my_mac[4], my_mac[5], peer_clock);
  
#ifdef HAS_LORA
  int rssi;
  double snr;
# ifdef USE_RADIO_LIB
  rssi = (int) radio.getRSSI(), snr = radio.getSNR();
# else
  rssi = LoRa.packetRssi(), snr = LoRa.packetSnr();
# endif
  Serial.printf(" rssi=%ddBm snr=%.1f", rssi, snr);
  sprintf(buf+strlen(buf), " rssi=%ddBm snr=%.1f", rssi, snr);
#endif
  Serial.printf("\r\n");

#ifdef HAS_GPS
  if (gps.location.isValid())
    sprintf(buf+strlen(buf), " gps=%.8g,%.8g,%g", gps.location.lat(),
            gps.location.lng(), gps.altitude.meters());
#endif

  // append what we received, in brackets
  sprintf(buf+strlen(buf), " [%s]", str);
  // we log the received reply as 'A'
  peers_log_wr(buf);

#ifdef HAS_LORA
  str[6] = '\0';
  _update_peer(str+2, rssi, snr);
#endif
}


void PeersClass::_update_peer(char *id, int rssi, float snr)
{
  long oldest = 0;
  int i, oldest_i = -1;

  for (i = 0; i < MAX_HEARD_PEERS; i++) {
    if (heard_peers[i].id[0] == '\0' || !strcmp(heard_peers[i].id, id))
      break;
    if (oldest == 0 || oldest < heard_peers[i].when) {
      oldest = heard_peers[i].when;
      oldest_i = i;
    }
  }
  if (i >= MAX_HEARD_PEERS) {
    if (oldest_i == -1) // this should not happen
      return;
    i = oldest_i;
  }
  strcpy(heard_peers[i].id, id);
  heard_peers[i].when = millis();
  heard_peers[i].rssi = rssi;
  heard_peers[i].snr = snr;

  theUI->heard_peer(id, rssi, snr);
}

double PeersClass::get_peer_density()
{
  return avg_peer_density;
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
