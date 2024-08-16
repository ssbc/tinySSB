// sched.cpp

#include "tinySSBlib.h"


const char *pkt_origin_gosn = "GosN"; // GOset novelty pkt
const char *pkt_origin_gosc = "GosC"; // GOset claim pkt
const char *pkt_origin_preq = "PreQ"; // peer ping request
const char *pkt_origin_prep = "PreP"; // peer ping reply
const char *pkt_origin_data = "Data"; // append-only log entry
const char *pkt_origin_chnk = "Chnk"; // side chain pkt
const char *pkt_origin_dreq = "DreQ"; // want vector
const char *pkt_origin_creq = "CreQ"; // chunk vector

enum {
  PROBE_GOSET = 0,
  PROBE_PEERS,
  PROBE_WANT,
  PROBE_CHUNK
};
  
SchedClass::SchedClass(probe_fct_t g, probe_fct_t p,
                       probe_fct_t w, probe_fct_t c)
{
  probe[PROBE_GOSET] = g, probe[PROBE_PEERS] = p;
  probe[PROBE_WANT] = w,  probe[PROBE_CHUNK] = c;
  curr_slot = 0;
  memset(slots, 0, sizeof(slots));
  memset(packets, 0, sizeof(packets));
  next_slot_millis = millis();
  slots[2]           = -1; // GOset, start immediately
  slots[(int)(5*SCHED_PPS)] = -2; // peers, start after 5 sec
  slots[(int)(2*SCHED_PPS)] = -3; // want vect, start after 3 sec
  slots[(int)(4*SCHED_PPS)] = -4; // chnk vect, start after 6 sec
}


void SchedClass::tick()
{
  if (millis() < next_slot_millis)
    return;
  next_slot_millis = millis() + 1000 / (SCHED_PPS) + esp_random() % 300;
  /*
  double d = thePeers->get_peer_density();
  if (d > 2.5) {
    if (esp_random() & 1) // 50%
      next_slot_millis += 2 * 1000 / (SCHED_PPS);
    else if (d > 1.5) {
      if (esp_random() & 1) // 50%
        next_slot_millis += 1000 / (SCHED_PPS);
    }
  }
  */
  int ndx = slots[curr_slot];
  slots[curr_slot] = 0;
  curr_slot = (curr_slot + 1) % (int)(SCHED_SEC * SCHED_PPS);
  if (ndx == 0) { // idle
    theRepo->loop();
    return;
  }
  if (ndx > 0) { // pkt slot
    ndx--;
    // Serial.printf("before io_send()\r\n");
    io_send(packets[ndx], lengths[ndx], faces[ndx], origins[ndx]);
    // Serial.printf("after io_send()\r\n");
    free(packets[ndx]);
    packets[ndx] = NULL;
    return;
  }

  ndx = -ndx - 1;
  // Serial.printf("sched tick, curr=%d, ndx=%d\r\n", curr_slot, ndx);
  // else it's one of the recurrent advertizers (g/p/v/c):
  if (probe[ndx] == NULL)
    return;
  unsigned char *pkt = NULL;
  unsigned short len;
  unsigned short next_millis;
  const char *cp;
  probe[ndx](&pkt, &len, &next_millis, &cp);
  // Serial.println("  after probe");
  if (pkt != NULL) { // our slot, use it
    // Serial.printf("  probe %d has pkt: %d\r\n", ndx, len);
    io_send(pkt, len, NULL, cp);
    // Serial.printf("  after probe\r\n");
    free(pkt);
  }
  // Serial.println("  probe has no packet");
  int i = _find_free_slot(next_millis);
  // Serial.printf("  new slot is %d\r\n", i);
  if (i >= 0)
    slots[i] = -ndx - 1;
  else
    Serial.println("oops, no free slot!");
  // check next_slot_millis, potentially delay?
}

  
void SchedClass::schedule_asap(unsigned char *pkt, unsigned short len,
                               unsigned char *dmx, struct face_s *f,
                               const char *origin)
/*
  if dmx==NULL then we assume that *pkt is a malloced buffer that we will free
  else *pkt as well as *dmx are memory zone that we have to copy here
 */
{
  // Serial.printf("   sched_asap %p %d %s\r\n",
  //               pkt, len, origin ? origin : "????");
  if (dmx != NULL) {
    unsigned char *tmp = (unsigned char*) malloc(len + DMX_LEN);
    memcpy(tmp, dmx, DMX_LEN);
    memcpy(tmp + DMX_LEN, pkt, len);
    pkt = tmp;
    len += DMX_LEN;
  }
  int i, j = -1;
  for (i = 0; i < SCHED_MAX; i++) {
    if (packets[i] == NULL)
      j = i;
    else if (lengths[i] == len && !memcmp(packets[i], pkt, len)) { // dup
    Serial.println("packet is a dup, not adding to the out queue");
      free(pkt);
      return;
    }
  }
  if (j == -1) {
    Serial.println("packet store is full, packet was dropped");
    free(pkt);
  } else {
    i = _find_free_slot(0);
    if (i >= 0) {
      packets[j] = pkt;
      lengths[j] = len;
      origins[j] = origin;
      faces[j] = f;
      slots[i] = j+1;
    } else {
      Serial.println("no free scheduling slot, packet was dropped");
      free(pkt);
    }
  }
}


int SchedClass::_find_free_slot(int delay_millis)
{
  // Serial.printf("find_free_slot %d\r\n", delay_millis);

  int max_slot = (SCHED_SEC * 1000 - delay_millis) * SCHED_PPS / 1000;
  int pos = curr_slot + delay_millis * SCHED_PPS / 1000;
  for (int i = 0; i < max_slot; i++) {
    int j = (pos + i) % (int)(SCHED_SEC * SCHED_PPS);
    if (slots[j] == 0)
      return j;
  }
  return -1;
}


// eof
