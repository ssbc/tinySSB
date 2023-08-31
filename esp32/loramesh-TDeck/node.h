// node.h

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// #include <lwip/def.h> // htonl()


unsigned int node_next_vector; // time when one of the two next vectors should be sent

long packet_proc_time;
int packet_proc_cnt;
long chunk_proc_time;
int chunk_proc_cnt;

void incoming_want_request(unsigned char *buf, int len, unsigned char *aux, struct face_s *f)
{
  struct bipf_s *lptr = bipf_loads(buf + DMX_LEN, len - DMX_LEN);
  if (lptr == NULL || lptr->typ != BIPF_LIST || lptr->cnt < 1 ||
                                                lptr->u.list[0]->typ != BIPF_INT) {
      Serial.printf("   =W formatting error\r\n");
      return;
  }
  struct bipf_s **lst = lptr->u.list;
  int offs = lst[0]->u.i;
  int credit = 3;
  // we should send log entries from different feeds, if possible.
  // otherwise a lost tSSB packet will render the subsequent packets useless
  // wherefore we create a frontier copy
  short* seq_copy = (short*) calloc(lptr->cnt, sizeof(short));
  for (int i = 1; i < lptr->cnt; i++)
    if (lst[i]->typ == BIPF_INT)
      seq_copy[i] = lst[i]->u.i;
  char found_something = 1;
  while (credit > 0 && found_something) {
    found_something = 0;
    for (int i = 1; credit > 0 && i < lptr->cnt; i++) {
      if (seq_copy[i] == 0) continue;
      int fNDX = (offs + i-1) % theGOset->goset_len;
      unsigned char *fid = theGOset->get_key(fNDX);
      int seq = seq_copy[i];
      fishForNewLoRaPkt();
      unsigned char *pkt = theRepo->fid2replica(fid)->get_entry_pkt(seq);
      if (pkt == NULL)
        continue;
      Serial.printf("  repo.read(%d.%d) -> %s..\r\n", fNDX, seq, to_hex(pkt, 8));
      // io_enqueue(pkt, TINYSSB_PKT_LEN);
      unsigned char *tmp = (unsigned char*) malloc(TINYSSB_PKT_LEN);
      memcpy(tmp, pkt, TINYSSB_PKT_LEN);
      theSched->schedule_asap(tmp, TINYSSB_PKT_LEN, NULL, f);
 
      found_something++;
      seq_copy[i]++;
      credit--;
    }
  }
  String v = "[";
  for (int i = 1; i < lptr->cnt; i++) {
    if (lst[i]->typ != BIPF_INT)
      continue;
    int fNDX = (offs + i-1) % theGOset->goset_len;
    int seq = lst[i]->u.i;
    v += " " + String(fNDX) + "." + String(seq);
    while (seq_copy[i]-- > seq)
      v += "*";
  }
  Serial.println("   =W " + v + " ]");
  bipf_free(lptr);
  free(seq_copy);
  return;
}

void incoming_chnk_request(unsigned char *buf, int len, unsigned char *aux, struct face_s *f)
{
  struct bipf_s *lptr = bipf_loads(buf + DMX_LEN, len - DMX_LEN);
  if (lptr == NULL || lptr->typ != BIPF_LIST) {
    Serial.printf("   =C formatting error\r\n");
    return;
  }
  int err_cnt = 0;
  struct bipf_s **slpptr = lptr->u.list;
  // FIXME: should assemble sequence of chunks from different sidechains, if possible.
  // otherwise a lost tSSB packet will render the subsequent chained packets useless
  short* cnr_copy = (short*) malloc(lptr->cnt * sizeof(short));
  for (int i = 0; i < lptr->cnt; i++, slpptr++) {
    cnr_copy[i] = -1;
    if ((*slpptr)->typ == BIPF_LIST && (*slpptr)->cnt >= 3 &&
        (*slpptr)->u.list[2]->typ == BIPF_INT)
      cnr_copy[i] = (*slpptr)->u.list[2]->u.i;
  }
  int credit = 3;
  char found_something = 1;
  while (credit > 0 && found_something) {
    found_something = 0;
    slpptr = lptr->u.list;
    for (int i = 0; i < lptr->cnt; i++, slpptr++) {
      if ((*slpptr)->typ != BIPF_LIST || (*slpptr)->cnt < 3) {
        err_cnt++;
        continue;
      }
      struct bipf_s **lst = (*slpptr)->u.list;
      if (lst[0]->typ != BIPF_INT || lst[1]->typ != BIPF_INT || lst[2]->typ != BIPF_INT) {
        err_cnt++;
        continue;
      }
        // ||
        //         (*slpptr)->u.list[0]->typ != BIPF_INT || (*slpptr)->u.list[1]->typ != BIPF_INT ||
        //         (*slpptr)->u.list[2]->typ != BIPF_INT)
        // ||
        //         (*slpptr)->u.list[0]->typ != BIPF_INT || (*slpptr)->u.list[1]->typ != BIPF_INT ||
        //         (*slpptr)->u.list[2]->typ != BIPF_INT)
    // int fNDX = (*slpptr)->u.list[0]->u.i;
    // int seq = (*slpptr)->u.list[1]->u.i;
    // int cnr = (*slpptr)->u.list[2]->u.i; // chunk nr
      int fNDX = lst[0]->u.i;
      int seq = lst[1]->u.i;
      int cnr = cnr_copy[i]; // chunk nr
      // v += (v.length() == 0 ? "[ " : " ") + String(fNDX) + "." + String(seq) + "." + String(cnr);
    // Serial.printf(" %d.%d.%d", fNDX, seq, cnr);
      unsigned char *fid = theGOset->get_key(fNDX);
      unsigned char *pkt = theRepo->fid2replica(fid)->get_entry_pkt(seq);
      if (pkt == NULL || pkt[DMX_LEN] != PKTTYPE_chain20) continue;
      int szlen = 4;
      int sz = bipf_varint_decode(pkt, DMX_LEN + 1, &szlen);
      if (sz <= 48-szlen-HASH_LEN) {
        Serial.printf("   -- no side chain for %d.%d? sz=%d\r\n", fNDX, seq, sz);
        Serial.printf("      content: %s\r\n", to_hex(pkt+DMX_LEN+1+szlen, sz));
        continue;
      }
      int max_chunks = (sz - (48 - szlen) + 99) / 100;
      if (cnr > max_chunks) {
        // Serial.printf("   -- chunk nr > maxchunks (%d.%d.%d > %d)\r\n", fNDX, seq, cnr, max_chunks);
        continue;
      }
      fishForNewLoRaPkt();
      unsigned char *chunk = theRepo->fid2replica(fid)->get_chunk_pkt(seq, cnr);

      if (chunk == NULL) { // missing content
        // Serial.printf("   -- cannot load chunk %d.%d.%d\r\n", fNDX, seq, cnr);
        continue;
      }
      //Serial.printf("   have chunk %d.%d.%d\r\n", fNDX, seq, cnr);
      // io_enqueue(chunk, TINYSSB_PKT_LEN, NULL, f);
      unsigned char *tmp = (unsigned char*) malloc(TINYSSB_PKT_LEN);
      memcpy(tmp, pkt, TINYSSB_PKT_LEN);
      theSched->schedule_asap(tmp, TINYSSB_PKT_LEN);
       
      found_something++;
      cnr_copy[i]++; // chunk nr
      credit--;
    }
  }
  String v = "[";
  slpptr = lptr->u.list;
  for (int i = 0; i < lptr->cnt; i++, slpptr++) {
    if ((*slpptr)->typ != BIPF_LIST || (*slpptr)->cnt < 3)
      continue;
    struct bipf_s **lst = (*slpptr)->u.list;
    if (lst[0]->typ != BIPF_INT || lst[1]->typ != BIPF_INT || lst[2]->typ != BIPF_INT)
      continue;
    v += " " + String(lst[0]->u.i) + "." + lst[1]->u.i + "." + lst[2]->u.i;
    while (cnr_copy[i]-- > lst[2]->u.i)
      v += "*";
  }
  Serial.println("   =C " + v + " ]");

  bipf_free(lptr);
  free(cnr_copy);
  if (err_cnt != 0)
    Serial.printf("   =C contains formatting errors\r\n");
  return;
}

void incoming_pkt(unsigned char *buf, int len, unsigned char *fid, struct face_s *f)
{
  // Serial.println("   incoming log entry");
  if (len != TINYSSB_PKT_LEN) return;
  unsigned long now = millis();
  ReplicaClass *r = theRepo->fid2replica(fid);
  if (r->ingest_entry_pkt(buf)) {
    theRepo->want_is_valid = 0;
    theRepo->entry_cnt++;
    theDmx->arm_dmx(buf); // remove old DMX handler for this packet

    // install handler for next pkt:
    unsigned char dmx_val[DMX_LEN];
    int ns = r->get_next_seq(dmx_val);
    Serial.printf("   appended %d.%d\r\n", theGOset->_key_index(fid), ns-1);
    fishForNewLoRaPkt();
    // int ndx = theGOset->_key_index(fid);
    theDmx->arm_dmx(dmx_val, incoming_pkt, r->fid, /*ndx,*/ ns);

    struct bipf_s *p = r->get_next_in_chain(ns-1);
    if (p != NULL) {
      theRepo->chnk_is_valid = 0;
      // Serial.printf("   .. new sidechain for seq=%d\r\n", ns-1);
      // [cnr, rem, hptr, pos]
      theDmx->arm_blb(p->u.list[2]->u.buf, incoming_chunk, r->fid,
                   ns-1, 0, p->u.list[1]->u.i);
    }
  } else
    Serial.printf("   .. not a valid pkt\r\n");
  int delta = millis() - now;
  Serial.printf("  .. stored in %d msec\r\n", delta);
  packet_proc_time += delta;
  packet_proc_cnt++;
}

void incoming_chunk(unsigned char *buf, int len, int blbt_ndx, struct face_s *f)
{
  // Serial.println("   incoming chunk");
  if (len != TINYSSB_PKT_LEN)
    return;
  unsigned long now = millis();

  char is_valid = 0;
  struct blb_s *bp = theDmx->blbt + blbt_ndx;
  for (struct chain_s *tp = bp->front; tp; tp = tp->next) {
    int ndx = theGOset->_key_index(tp->fid);
    int next_cnr;
    if (theRepo->fid2replica(tp->fid)->ingest_chunk_pkt(buf, tp->seq, &next_cnr)) {
      is_valid = 1;
      Serial.printf("   persisted chunk %d.%d.%d\r\n", ndx, tp->seq, tp->cnr);
      theRepo->chnk_is_valid = 0;
      theRepo->chunk_cnt++;
      if (next_cnr > 0)
        theDmx->arm_blb(buf+100, incoming_chunk, tp->fid, tp->seq, tp->cnr+1, tp->last_cnr);
    } else 
      Serial.printf("  invalid chunk %d.%d.%d/%d or file problem?\r\n",
                    ndx, tp->seq, tp->cnr, tp->last_cnr);
    fishForNewLoRaPkt();
    // io_dequeue();
    theSched->tick();
  }
  if (is_valid)
    theDmx->arm_blb(bp->h); // remove old CHUNK handler for this hash val
  // Serial.printf("   end of persisting chunk\r\n");

  int delta = millis() - now;
  Serial.printf("   .. stored in %d msec\r\n", delta);
  chunk_proc_time += delta;
  chunk_proc_cnt++;
}

/*
void node_tick()
{
  static unsigned char turn; // alternate between requesting log entries an chunks

  unsigned long now = millis();
  if (now < node_next_vector) // && (node_next_vector-now) < 2*NODE_ROUND_LEN) // FIXME: test whether wraparound works
    return;
  node_next_vector = now + NODE_ROUND_LEN + esp_random() % 500;

  lora_pps = 0.75 * lora_pps + 0.25 * 1000.0 * lora_pkt_cnt / NODE_ROUND_LEN / 2;
  // lora_pps = 1000.0 * lora_pkt_cnt / NODE_ROUND_LEN;
  lora_pkt_cnt = 0;
 
  Serial.printf("-- t=%d.%03d ", now/1000, now%1000);
#if defined(AXP_DEBUG)
  Serial.printf("%1.04gV ", axp.getBattVoltage()/1000);
#endif
  //  Serial.printf("|dmxt|=%d |blbt|=%d |feeds|=%d |entries|=%d |chunks|=%d |freeHeap|=%d pps=%1.2f\r\n",
  //                theDmx->dmxt_cnt, theDmx->blbt_cnt, theRepo->feed_cnt, theRepo->entry_cnt, theRepo->chunk_cnt, ESP.getFreeHeap(), lora_pps);
  Serial.printf("%dF%dE%dC |dmxt|=%d |blbt|=%d |freeHeap|=%d lora_sent=%d lora_rcvd=%d pps=%1.2f\r\n",
                theRepo->rplca_cnt, theRepo->entry_cnt, theRepo->chunk_cnt,
                theDmx->dmxt_cnt, theDmx->blbt_cnt, ESP.getFreeHeap(),
                lora_sent_pkts, lora_rcvd_pkts, lora_pps);
  if (packet_proc_time != 0 && chunk_proc_time != 0) {
    Serial.printf("   t/packet=%6.2g t/chunk=%6.2g (msec)\r\n",
                  (double)packet_proc_time / packet_proc_cnt,
                  (double)chunk_proc_time / chunk_proc_cnt);
  }

  if (theGOset->goset_len == 0)
    return;

  // Serial.printf("turn=%d t=%d.%0d\r\n", turn, now/1000, now%1000);
  turn = 1 - turn;
  if (turn) {
    theRepo->mk_want_vect();
    if (theRepo->want_len > 0) {
      io_enqueue(theRepo->want_vect, theRepo->want_len, theDmx->want_dmx);
    }
  } else { // chunks:
    theRepo->mk_chnk_vect();
    if (theRepo->chnk_len > 0) {
      io_enqueue(theRepo->chnk_vect, theRepo->chnk_len, theDmx->chnk_dmx);
    }
  }

  fishForNewLoRaPkt();
  io_dequeue();
}
*/

void probe_for_want_vect(unsigned char **pkt,
                         unsigned short *len,
                         unsigned short *reprobe_in_millis)
{
  *reprobe_in_millis = NODE_ROUND_LEN/2 + esp_random() % 500;

  *pkt = NULL;
  if (theGOset->goset_len == 0)
    return;
  theRepo->mk_want_vect();
  if (theRepo->want_len <= 0)
    return;
  Serial.println("   prepare WANT vect");
  *len = 7 + theRepo->want_len;
  *pkt = (unsigned char*) malloc(*len);
  memcpy(*pkt, theDmx->want_dmx, 7);
  memcpy(*pkt + 7, theRepo->want_vect, theRepo->want_len);
}


void probe_for_chnk_vect(unsigned char **pkt,
                         unsigned short *len,
                         unsigned short *reprobe_in_millis)
{
  // Serial.println("probe_for_chnk_vect");
  *reprobe_in_millis = NODE_ROUND_LEN/2 + esp_random() % 500;

  *pkt = NULL;
  if (theGOset->goset_len == 0)
    return;
  theRepo->mk_chnk_vect();
  if (theRepo->chnk_len <= 0)
    return;
  Serial.println("   prepare CHNK vect");
  *len = 7 + theRepo->chnk_len;
  *pkt = (unsigned char*) malloc(*len);
  memcpy(*pkt, theDmx->chnk_dmx, 7);
  memcpy(*pkt + 7, theRepo->chnk_vect, theRepo->chnk_len);
}

// eof
