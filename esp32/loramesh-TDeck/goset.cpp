// goset.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// Grow-Only Set

#include <lwip/def.h>
#include <string.h>

#include "config.h"

int _cmp_cnt(const void *a, const void *b)
{
  return ((struct claim_s*)a)->cnt - ((struct claim_s*)b)->cnt;
}

int _cmp_key32(const void *a, const void *b)
{
  return memcmp(a,b,GOSET_KEY_LEN);
}

unsigned char* GOsetClass::_xor(int lo, int hi)
{
  static unsigned char sum[GOSET_KEY_LEN];
  // cpu_set_fast();
  memset(sum, 0, GOSET_KEY_LEN);
  unsigned char *cp = this->goset_keys + lo*GOSET_KEY_LEN;
  for (int i = lo; i <= hi; i++)
    for (int j = 0; j < GOSET_KEY_LEN; j++)
      sum[j] ^= *cp++;
  // cpu_set_slow();
  return sum;  
}

int GOsetClass::isZero(unsigned char *h, int len)
{
  static unsigned char zero[GOSET_KEY_LEN];
  return !memcmp(zero, h, len);
}

unsigned char* GOsetClass::_mkNovelty(unsigned char *start, int cnt)
{
  static unsigned char pkt[NOVELTY_LEN];
  pkt[0] = 'n';
  memcpy(pkt+1, start, GOSET_KEY_LEN);
  return pkt;
}

unsigned char* GOsetClass::_mkClaim(int lo, int hi)
{
  static struct claim_s claim;
  claim.typ = 'c';
  memcpy(claim.lo, this->goset_keys + lo*GOSET_KEY_LEN, GOSET_KEY_LEN);
  memcpy(claim.hi, this->goset_keys + hi*GOSET_KEY_LEN, GOSET_KEY_LEN);
  memcpy(claim.xo, this->_xor(lo, hi), GOSET_KEY_LEN);
  claim.cnt = hi - lo + 1;
  return (unsigned char*) &claim;
}

unsigned char* GOsetClass::_mkZap() // fill buffer with zap packet
{
  static unsigned char pkt[DMX_LEN + ZAP_LEN];
  memcpy(pkt, theDmx->goset_dmx, DMX_LEN);
  memcpy(pkt + DMX_LEN, &(this->zap), ZAP_LEN);
  return pkt;
}

int GOsetClass::_key_index(unsigned char *key)
{
  for (int i = 0; i < this->goset_len; i++)
    if (!memcmp(this->goset_keys + i*GOSET_KEY_LEN, key, GOSET_KEY_LEN))
      return i;
  Serial.println("** BUG: index negative for " + String(to_hex(key, GOSET_KEY_LEN, 0)));
  return -1;
}

unsigned char* GOsetClass::get_key(int ndx)
{
  ndx = ndx % this->goset_len;
  return this->goset_keys + ndx * GOSET_KEY_LEN;
}

void GOsetClass::_add_pending(unsigned char *claim)
{
  if (this->pending_c_cnt >= MAX_PENDING)
    return;
  for (int i = 0; i < this->pending_c_cnt; i++)
    if (!memcmp(this->pending_claims + i, claim, CLAIM_LEN))
      return;
  // Serial.println(String("  adding claim ") + to_hex(claim+65,32));
  memcpy(this->pending_claims + this->pending_c_cnt, claim, CLAIM_LEN);
  this->pending_c_cnt++;
}

// ----------------------------------------------------------------------------

void GOsetClass::dump()
{
  Serial.printf("GOset: %d keys\r\n", this->goset_len);
  for (int i = 0; i < this->goset_len; i++) {
    Serial.printf("  %2d %s\r\n", i,
                  to_hex(this->goset_keys + i * GOSET_KEY_LEN, GOSET_KEY_LEN, 0));
  }
}

void GOsetClass::add(unsigned char *key)
{
  if (isZero(key, GOSET_KEY_LEN))
    return;
  for (int i = 0; i < this->goset_len; i++)
    if (!memcmp(this->goset_keys+i*GOSET_KEY_LEN, key, GOSET_KEY_LEN)) {
      // Serial.println(String("key already in set: ") + to_hex(key, GOSET_KEY_LEN));
      return;
    }
  if (this->goset_len >= GOSET_MAX_KEYS) {
    Serial.printf("   too many keys: %d\n", this->goset_len);
    return;
  }

  memcpy(this->goset_keys + this->goset_len * GOSET_KEY_LEN, key, GOSET_KEY_LEN);
  this->goset_len++;
  qsort(this->goset_keys, this->goset_len, GOSET_KEY_LEN, _cmp_key32);

  theRepo->add_replica(key);

  if (this->goset_len >= this->largest_claim_span) { // only rebroadcast if we are up to date
    if (this->novelty_credit-- > 0) {
      // io_enqueue(_mkNovelty(key, 1), NOVELTY_LEN, theDmx->goset_dmx, NULL);
      theSched->schedule_asap(_mkNovelty(key, 1), NOVELTY_LEN,
                              theDmx->goset_dmx, NULL);
    } else {
      if (this->pending_n_cnt < MAX_PENDING)
        memcpy(this->pending_novelty + this->pending_n_cnt++, _mkNovelty(key, 1), NOVELTY_LEN);
    }
  }

  Serial.printf("   G: added key %s, len=%d\r\n", to_hex(key, GOSET_KEY_LEN, 0), this->goset_len);
  this->populate(NULL); // recomputes DMX
}

void GOsetClass::populate(unsigned char *key)
{
  if (key == NULL) {
    qsort(this->goset_keys, this->goset_len, GOSET_KEY_LEN, _cmp_key32);
    // init the WANT dmx handler for this GOset
    memcpy(this->goset_state, this->_xor(0, this->goset_len-1), GOSET_KEY_LEN);
    theDmx->set_want_dmx();
    return;
  }
  if (this->goset_len >= GOSET_MAX_KEYS) {
    Serial.printf("   G: too many keys: %d\n", this->goset_len);
    return;
  }
  memcpy(this->goset_keys + this->goset_len * GOSET_KEY_LEN, key, GOSET_KEY_LEN);
  this->goset_len++;
}

void GOsetClass::rx(unsigned char *pkt, int len, unsigned char *aux, struct face_s *f)
{
  // struct face_s *f,
  // struct goset_s *gp = theGOset;
  pkt += DMX_LEN;
  len -= DMX_LEN;

  if (pkt[0] == 'n' && len == NOVELTY_LEN) {
    Serial.printf("   =G.NOV %s\r\n", to_hex(pkt+1, GOSET_KEY_LEN, 0));
    this->add(pkt+1);
    return;
  }
  if (pkt[0] == 'z' && len == ZAP_LEN) {
    Serial.println("   =G.ZAP");
    unsigned long now = millis();
    if (this->zap_state == 0) {
      Serial.println("   ZAP phase I starts");
      memcpy(&this->zap, pkt, ZAP_LEN);
      this->zap_state = now + ZAP_ROUND_LEN;
      this->zap_next = now;
    }
    return;
  }
  if (pkt[0] != 'c' || len != CLAIM_LEN) {
    Serial.printf("   =G.?? t=%c\r\n", pkt[0]);
    return;
  }
  struct claim_s *cp = (struct claim_s *) pkt;
  Serial.printf("   =G.CLM xo=%s, |span|=%d\r\n", to_hex(cp->xo, GOSET_KEY_LEN, 0), cp->cnt);
  if (isZero(cp->lo, GOSET_KEY_LEN)) // remove this clause
    return;
  if (cp->cnt > this->largest_claim_span)
    this->largest_claim_span = cp->cnt;
  this->add(cp->lo);
  this->add(cp->hi);
  this->_add_pending(pkt);
}

void GOsetClass::tick()
{
  unsigned long now = millis();
  if (this->zap_state != 0) {
    if (now > this->zap_state + ZAP_ROUND_LEN) { // two rounds after zap started
      int ndx = ntohl(this->zap.ndx);
      Serial.println("ZAP phase II ended, resetting now, ndx=" + String(ndx));
      if (ndx == -1)
        theRepo->reset(NULL);
      else {
        char path[90];
        unsigned char *fid = this->goset_keys + ndx * FID_LEN;
        sprintf(path, "%s/%s", FEED_DIR, to_hex(fid, FID_LEN, 0));
        theRepo->reset(path);
      }
    }
    if (now < this->zap_state && now > this->zap_next) { // phase I
      Serial.printf("   sending zap message (%d bytes)\r\n", DMX_LEN + ZAP_LEN);
      io_send(this->_mkZap(), DMX_LEN + ZAP_LEN, NULL);
      this->zap_next = now + 1000;
    }
  }
  if (now < this->next_round) // && (this->next_round-now) < 2*GOSET_ROUND_LEN) // FIXME: test whether wraparound works
    return;
  this->next_round = now + GOSET_ROUND_LEN + esp_random() % 1000;

  if (this->goset_len == 0)
    return;
  if (this->goset_len == 1) { // can't send a claim, send the one key as novelty
    // io_enqueue(_mkNovelty(this->goset_keys, 1), NOVELTY_LEN, theDmx->goset_dmx, NULL);
    theSched->schedule_asap(_mkNovelty(this->goset_keys, 1), NOVELTY_LEN,
                            theDmx->goset_dmx, NULL);
    return;
  }

  while (this->novelty_credit-- > 0 && this->pending_n_cnt > 0) {
    // io_enqueue((unsigned char*) this->pending_novelty, NOVELTY_LEN, theDmx->goset_dmx, NULL);
    theSched->schedule_asap((unsigned char*) this->pending_novelty, NOVELTY_LEN,
                            theDmx->goset_dmx, NULL);
    memmove(this->pending_novelty, this->pending_novelty+1, NOVELTY_LEN * --this->pending_n_cnt);
  }
  this->novelty_credit = NOVELTY_PER_ROUND;

  unsigned char *claim = this->_mkClaim(0, this->goset_len-1);
  if (memcmp(this->goset_state, claim+65, GOSET_KEY_LEN)) { // GOset changed
    memcpy(this->goset_state, claim+65, GOSET_KEY_LEN);
    theDmx->set_want_dmx();
  }
  Serial.printf("   G: mk [xo=%s, ||=%d] %d\r\n", to_hex(claim+65, GOSET_KEY_LEN),
                this->goset_len, CLAIM_LEN);
  // io_enqueue(claim, CLAIM_LEN, theDmx->goset_dmx, NULL);
  theSched->schedule_asap(claim, CLAIM_LEN, theDmx->goset_dmx, NULL);
  
  // sort pending entries, smallest first
  qsort(this->pending_claims, this->pending_c_cnt, CLAIM_LEN, _cmp_cnt);
  int max_ask = ASK_PER_ROUND;
  int max_help = HELP_PER_ROUND;

  struct claim_s *cp = this->pending_claims;
  for (int i = 0; i < this->pending_c_cnt; i++) {
    int lo = this->_key_index(cp->lo);
    int hi = this->_key_index(cp->hi);
    struct claim_s *partial = (struct claim_s*) this->_mkClaim(lo, hi);

    if (cp->cnt == 0 || lo < 0 || hi < 0 || lo > hi // eliminate claims that match or are bogous
        // || !memcmp(partial->xo, this->goset_state, GOSET_KEY_LEN)
        || !memcmp(partial->xo, cp->xo, GOSET_KEY_LEN) ) {
      memmove(cp, cp+1, (this->pending_c_cnt - i - 1)*CLAIM_LEN);
      this->pending_c_cnt--;
      i--;
      continue;
    } 
    // Serial.println("  not eliminated " + String(lo,DEC) + " " + String(hi,DEC) + " " + String(cp->cnt));
    if (partial->cnt <= cp->cnt) { // ask for help, but only for smallest entry, and only once in this round
      if (max_ask-- > 0) {
        Serial.printf("   asking for help cnt=%d [%d..%d]\r\n",
                      partial->cnt,
                      this->_key_index(partial->lo),
                      this->_key_index(partial->hi));
        // io_enqueue((unsigned char*) partial, CLAIM_LEN, theDmx->goset_dmx, NULL);
        theSched->schedule_asap((unsigned char*) partial, CLAIM_LEN,
                                theDmx->goset_dmx, NULL);
      }
      if (partial->cnt < cp->cnt) {
        cp++;
        // memmove(cp, cp+1, (this->pending_c_cnt - i - 1)*CLAIM_LEN); // remove this claim we reacted on
        // this->pending_c_cnt--;
        // i--;
        // // do not help if we have holes ...
        // max_help = 0;
        continue;
      }
    }
    if (max_help-- > 0) { // we have larger claim span, offer help (but limit # of claims)
      hi--, lo++;
      Serial.print("   offer help span=" + String(partial->cnt - 2));
      Serial.print(String(" ") + to_hex(this->goset_keys+lo*GOSET_KEY_LEN,4,0) + String(".."));
      Serial.println(String(" ") + to_hex(this->goset_keys+hi*GOSET_KEY_LEN,4,0) + String(".."));
      if (hi <= lo) {
        // io_enqueue(_mkNovelty(this->goset_keys+lo*GOSET_KEY_LEN, 1), NOVELTY_LEN, theDmx->goset_dmx, NULL);
        theSched->schedule_asap(_mkNovelty(this->goset_keys+lo*GOSET_KEY_LEN,1),
                                NOVELTY_LEN, theDmx->goset_dmx);
      } else if (hi - lo <= 2) { // span of 2 or 3
        // io_enqueue(this->_mkClaim(lo, hi), CLAIM_LEN, theDmx->goset_dmx, NULL);
        theSched->schedule_asap(this->_mkClaim(lo, hi), CLAIM_LEN,
                                theDmx->goset_dmx);
      } else { // split span in two intervals
        int sz = (hi+1 - lo) / 2;
        Serial.printf("   G: helping: [%d..%d], [%d..%d]\r\n",
                      lo, lo+sz-1, lo+sz, hi);
        // io_enqueue(this->_mkClaim(lo, lo+sz-1), CLAIM_LEN, theDmx->goset_dmx, NULL);
        theSched->schedule_asap(this->_mkClaim(lo, lo+sz-1), CLAIM_LEN,
                                theDmx->goset_dmx);
        // io_enqueue(this->_mkClaim(lo+sz, hi), CLAIM_LEN, theDmx->goset_dmx, NULL);
        theSched->schedule_asap(this->_mkClaim(lo+sz, hi), CLAIM_LEN,
                                theDmx->goset_dmx);        
      }
      memmove(cp, cp+1, (this->pending_c_cnt - i - 1)*CLAIM_LEN);
      this->pending_c_cnt--;
      i--;
      continue;
    }
    cp++;
    // option: trim pending claims?
    // this->pending_c_cnt = i+1;
    // break;
  }
  // make room for new claims
  if (this->pending_c_cnt > (MAX_PENDING-5))
    this->pending_c_cnt = MAX_PENDING-5;
  // if (this->pending_c_cnt > 2)
  //   this->pending_c_cnt = 2; // better: log2(largest_claim_span) ?
  if (this->pending_c_cnt > 0)
    Serial.printf("   G: len=%d, %d pending claims:\r\n", this->goset_len, this->pending_c_cnt);
  // unsigned char *heap = reinterpret_cast<unsigned char*>(sbrk(0));
  // Serial.println(String(", heap sbrk=") + to_hex((unsigned char *)&heap, sizeof(heap)));
  for (int i = 0; i < this->pending_c_cnt; i++)
    Serial.printf("     xor=%s, |span|=%d\r\n", to_hex(this->pending_claims[i].xo,32,0), this->pending_claims[i].cnt);
}

void GOsetClass::do_zap(int ndx)
{
  unsigned long now = millis();
  this->zap.typ = 'z';
  this->zap.ndx = htonl(ndx);
  for (int i=0; i < sizeof(this->zap.key)/sizeof(uint32_t); i++) {
    uint32_t r = esp_random();
    memcpy(this->zap.key + 4*i, (unsigned char*) &r, sizeof(uint32_t));
  }
  this->zap_state = now + ZAP_ROUND_LEN;
  this->zap_next = now + 1000;
  io_send(this->_mkZap(), DMX_LEN + ZAP_LEN, NULL);
}


static void _mk_goset_pkt(unsigned char **dptr, unsigned short *dlen,
                   unsigned char *src, unsigned short slen)
// mallocs, and prepends DMX field
{
  *dlen = 7 + slen;
  *dptr = (unsigned char*) malloc(*dlen);
  memcpy(*dptr, theDmx->goset_dmx, 7);
  memcpy(*dptr + 7, src, slen);
}

void GOsetClass::probe_for_goset_vect(unsigned char **pkt,
                                      unsigned short *len,
                                      unsigned short *reprobe_in_millis)
{
  // Serial.printf("probe goset len=%d\r\n", goset_len);
  *reprobe_in_millis = GOSET_ROUND_LEN/4 + esp_random() % 500;

  *pkt = NULL;
  if (goset_len == 0)
    return;

  if (last_round + GOSET_ROUND_LEN < millis()) {
    last_round = millis();

    if (goset_len == 1) { // can't send a claim, send the one key as novelty
      Serial.println("   G: mk one novelty");
      _mk_goset_pkt(pkt, len, _mkNovelty(goset_keys, 1), NOVELTY_LEN);
      return;
    }

    unsigned char *claim = _mkClaim(0, goset_len-1);
    if (memcmp(goset_state, claim+65, GOSET_KEY_LEN)) { // GOset changed
      memcpy(goset_state, claim+65, GOSET_KEY_LEN);
      theDmx->set_want_dmx();
    }
    Serial.printf("   G: mk full claim\r\n");
    _mk_goset_pkt(pkt, len, claim, CLAIM_LEN);
    return;
  }
  if (pending_c_cnt <= 0)
    return;
  Serial.println("   G: there are pending claims");

  // work on pending requests, start by sorting them
  // sort pending entries, smallest first
  qsort(pending_claims, pending_c_cnt, CLAIM_LEN, _cmp_cnt);
  int max_ask = ASK_PER_ROUND;
  int max_help = HELP_PER_ROUND;

  struct claim_s *cp = pending_claims;
  for (int i = 0; i < pending_c_cnt; i++) {
    int lo = _key_index(cp->lo);
    int hi = _key_index(cp->hi);
    struct claim_s *partial = (struct claim_s*) _mkClaim(lo, hi);

    if (cp->cnt == 0 || lo < 0 || hi < 0 || lo > hi // eliminate claims that match or are bogous
        // || !memcmp(partial->xo, this->goset_state, GOSET_KEY_LEN)
        || !memcmp(partial->xo, cp->xo, GOSET_KEY_LEN) ) {
      memmove(cp, cp+1, (pending_c_cnt - i - 1)*CLAIM_LEN);
      pending_c_cnt--;
      i--;
      continue;
    } 
    // Serial.println("  not eliminated " + String(lo,DEC) + " " + String(hi,DEC) + " " + String(cp->cnt));
    if (partial->cnt <= cp->cnt) { // ask for help, but only for smallest entry, and only once in this round
      if (max_ask-- > 0) {
        Serial.printf("   G: me asking for help cnt=%d [%d..%d]\r\n",
                      partial->cnt,
                      _key_index(partial->lo),
                      _key_index(partial->hi));
        {
          unsigned char *p;
          unsigned short l;
          _mk_goset_pkt(&p, &l, (unsigned char*) partial, CLAIM_LEN);
          // Serial.printf("     mk sub-claim, %dB\r\n", l);
          theSched->schedule_asap(p, l);
        }
        // io_enqueue((unsigned char*) partial, CLAIM_LEN, theDmx->goset_dmx, NULL);
      }
      if (partial->cnt < cp->cnt) {
        cp++;
        // memmove(cp, cp+1, (this->pending_c_cnt - i - 1)*CLAIM_LEN); // remove this claim we reacted on
        // this->pending_c_cnt--;
        // i--;
        // // do not help if we have holes ...
        // max_help = 0;
        continue;
      }
    }
    if (max_help-- > 0) { // we have larger claim span, offer help (but limit # of claims)
      hi--, lo++;
      Serial.print("   G: offer help span=" + String(partial->cnt - 2));
      Serial.print(String(" ") + to_hex(goset_keys+lo*GOSET_KEY_LEN,4,0) + String(".."));
      Serial.println(String(" ") + to_hex(goset_keys+hi*GOSET_KEY_LEN,4,0) + String(".."));
      if (hi <= lo) {
        // io_enqueue(_mkNovelty(this->goset_keys+lo*GOSET_KEY_LEN, 1), NOVELTY_LEN, theDmx->goset_dmx, NULL);
        unsigned char *p;
        unsigned short l;
        _mk_goset_pkt(&p, &l, _mkNovelty(goset_keys+lo*GOSET_KEY_LEN, 1),
                      NOVELTY_LEN);
        Serial.printf("   G: schedule a sub-claim, %dB\r\n", l);
        theSched->schedule_asap(p, l);
      } else if (hi - lo <= 2) { // span of 2 or 3
        // io_enqueue(this->_mkClaim(lo, hi), CLAIM_LEN, theDmx->goset_dmx, NULL);
        unsigned char *p;
        unsigned short l;
        _mk_goset_pkt(&p, &l, _mkClaim(lo, hi), CLAIM_LEN);
        Serial.printf("   G: schedule a sub-claim, %dB\r\n", l);
        theSched->schedule_asap(p, l);
      } else { // split span in two intervals
        int sz = (hi+1 - lo) / 2;
        Serial.printf("   G: helping with [%d..%d], [%d..%d]\r\n",
                      lo, lo+sz-1, lo+sz, hi);
        unsigned char *p;
        unsigned short l;
        // io_enqueue(this->_mkClaim(lo, lo+sz-1), CLAIM_LEN, theDmx->goset_dmx, NULL);
        _mk_goset_pkt(&p, &l, _mkClaim(lo, lo+sz-1), CLAIM_LEN);
        Serial.printf("   G: mk 1st sub-claim, %dB\r\n", l);
        theSched->schedule_asap(p, l);
        // io_enqueue(this->_mkClaim(lo+sz, hi), CLAIM_LEN, theDmx->goset_dmx, NULL);
        _mk_goset_pkt(&p, &l, _mkClaim(lo+sz, hi), CLAIM_LEN);
        Serial.printf("   G: mk 2nd sub-claim, %dB\r\n", l);
        theSched->schedule_asap(p, l);
      }
      memmove(cp, cp+1, (pending_c_cnt - i - 1)*CLAIM_LEN);
      pending_c_cnt--;
      i--;
      continue;
    }
    cp++;
    // option: trim pending claims?
    // this->pending_c_cnt = i+1;
    // break;
  }
  // make room for new claims
  if (pending_c_cnt > (MAX_PENDING-5))
    pending_c_cnt = MAX_PENDING-5;
  // if (this->pending_c_cnt > 2)
  //   this->pending_c_cnt = 2; // better: log2(largest_claim_span) ?
  if (pending_c_cnt > 0)
    Serial.printf("   |GOset|=%d, %d pending claims:\r\n", goset_len, pending_c_cnt);
  // unsigned char *heap = reinterpret_cast<unsigned char*>(sbrk(0));
  // Serial.println(String(", heap sbrk=") + to_hex((unsigned char *)&heap, sizeof(heap)));
  for (int i = 0; i < pending_c_cnt; i++)
    Serial.printf("     xor=%s, |span|=%d\r\n", to_hex(pending_claims[i].xo,32,0), pending_claims[i].cnt);
  
}


// eof
