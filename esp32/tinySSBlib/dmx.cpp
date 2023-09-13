// dmx.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

#include <stdio.h>
#include <string.h>

#include "tinySSBlib.h"

extern GOsetClass *theGOset;

extern void incoming_want_request(unsigned char* buf, int len, unsigned char* fid, struct face_s *);
extern void incoming_chnk_request(unsigned char* buf, int len, unsigned char* fid, struct face_s *);

// -----------------------------------------------------------------------

int DmxClass::_dmxt_index(unsigned char *dmx)
{
  for (int i = 0; i < this->dmxt_cnt; i++) {
    if (!memcmp(dmx, this->dmxt[i].dmx, DMX_LEN))
      return i;
  }
  return -1;
}

int DmxClass::_chkt_index(unsigned char *h)
{
  for (int i = 0; i < this->chkt_cnt; i++) {
    if (!memcmp(h, this->chkt[i].h, HASH_LEN))
      return i;
  }
  return -1;
}

void DmxClass::arm_dmx(unsigned char *dmx,
             void (*fct)(unsigned char*, int, unsigned char*, struct face_s*),
                       unsigned char *fid, /*int ndx,*/ int seq)
{
  int fndx = this->_dmxt_index(dmx);
  if (fct == NULL) { // del
    if (fndx != -1) {
      memmove(this->dmxt+fndx, this->dmxt+fndx+1,
              (this->dmxt_cnt - fndx - 1) * sizeof(struct dmx_s));
      this->dmxt_cnt--;
    }
    return;
  }
  if (fndx == -1) {
    if (this->dmxt_cnt >= DMXT_SIZE) {
      Serial.println("adm_dmx: dmxt is full");
      return; // full
    }
    fndx = this->dmxt_cnt++;
  }
  memcpy(this->dmxt[fndx].dmx, dmx, DMX_LEN);
  this->dmxt[fndx].fct = fct;
  this->dmxt[fndx].fid = fid;
  // this->dmxt[fndx].ndx = ndx;
  this->dmxt[fndx].seq = seq;
}

void DmxClass::arm_hsh(unsigned char *h,
             void (*fct)(unsigned char*, int, int, struct face_s*),
                       unsigned char *fid, int snr, int cnr, int is_last)
{
  int ndx = _chkt_index(h);
  struct hsh_s *bptr = ndx < 0 ? NULL : chkt + ndx;
  if (fct == NULL) { // remove the entry
    // Serial.printf("arm_hsh REMOVE\r\n");
    if (bptr != NULL) { // if it exists
      while (bptr->front) { // remove chain
        struct chain_s *tp = bptr->front->next;
        free(bptr->front);
        bptr->front = tp;
      }
      // compact table
      memmove(bptr, bptr+1, (chkt_cnt - ndx - 1) * sizeof(struct hsh_s));
      chkt_cnt--;
    }
    return;
  }
  if (bptr == NULL) {
    if (this->chkt_cnt == CHKT_SIZE) { // remove first (=oldest) entry
      // Serial.printf("  shrink chunk table\r\n");
      bptr = chkt;
      while (bptr->front) { // remove chain
        struct chain_s *tp = bptr->front->next;
        free(bptr->front);
        bptr->front = tp;
      }
      // compact table
      memmove(bptr, bptr+1, (chkt_cnt - 1) * sizeof(struct hsh_s));
      chkt_cnt--;
    }
    ndx = this->chkt_cnt++;
    bptr = chkt + ndx;
    // memset(bptr, 0, sizeof(struct hsh_s));
    memcpy(bptr->h, h, HASH_LEN);
    bptr->fct = fct;
    bptr->front = NULL;
  }
  // check whether we already have armed for this fid/snr/cnr combo
  struct chain_s *tp = bptr->front;
  while (tp) {
    if (tp->seq == snr && tp->cnr == cnr && !memcmp(tp->fid, fid, FID_LEN))
      return;
    tp = tp->next;
  }
  tp = (struct chain_s*) malloc(sizeof(struct chain_s));
  tp->fid = fid; // (unsigned char*) malloc(FID_LEN);
  tp->seq = snr;
  tp->cnr = cnr;
  tp->last_cnr = is_last;
  tp->next = bptr->front;
  bptr->front = tp;
}

void DmxClass::compute_dmx(unsigned char *dst, unsigned char *buf, int len)
{
  struct crypto_hash_sha256_state h;
  unsigned char out[crypto_hash_sha256_BYTES];
  crypto_hash_sha256_init(&h);
  crypto_hash_sha256_update(&h, (unsigned char*) DMX_PFX, strlen(DMX_PFX));
  crypto_hash_sha256_update(&h, buf, len);
  crypto_hash_sha256_final(&h, out);
  // Serial.printf("comput_dmx %s", to_hex((unsigned char*)DMX_PFX, DMX_LEN));
  // Serial.printf("%s\n", to_hex(buf, len));
  memcpy(dst, out, DMX_LEN);
  // Serial.printf(" --> dmx=%s\n", to_hex(dst, DMX_LEN));
}

int DmxClass::on_rx(unsigned char *buf, int len, unsigned char *h, struct face_s *f)
{
  int rc = -1;

  int ndx = this->_dmxt_index(buf);
  if (ndx >= 0) {
    // dmxt[ndx].fct(buf + DMX_LEN, len - DMX_LEN, dmxt[ndx].aux);
    this->dmxt[ndx].fct(buf, len, this->dmxt[ndx].fid, f);
    // return 0;  // try also the hash path (colliding DMX values so both handler must be served)
    rc = 0;
  }
  ndx = this->_chkt_index(h);
  if (ndx >= 0) {
    this->chkt[ndx].fct(buf, len, ndx, f);
    rc = 0;
  }

  return rc;
}

void DmxClass::set_want_dmx()
{
  this->arm_dmx(this->want_dmx, NULL); // undefine current handler
  this->arm_dmx(this->chnk_dmx, NULL); // undefine current handler
  unsigned char buf[4 + GOSET_KEY_LEN];

  memcpy(buf, "want", 4);
  memcpy(buf+4, theGOset->goset_state, GOSET_KEY_LEN);
  compute_dmx(this->want_dmx, buf, sizeof(buf));
  arm_dmx(this->want_dmx, incoming_want_request, NULL);
  Serial.println(String("   DMX for WANT is ") + to_hex(this->want_dmx, DMX_LEN, 0));

  memcpy(buf, "blob", 4); // FIXME: value is historic -- should be the string "chunk" for a next tinySSB protocol version 
  memcpy(buf+4, theGOset->goset_state, GOSET_KEY_LEN);
  compute_dmx(this->chnk_dmx, buf, sizeof(buf));
  arm_dmx(this->chnk_dmx, incoming_chnk_request, NULL);
  Serial.println(String("   DMX for CHNK is ") + to_hex(this->chnk_dmx, DMX_LEN, 0));
}

// eof
