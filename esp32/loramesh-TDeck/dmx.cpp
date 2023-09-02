// dmx.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

// FIXME: in the code, rename blob to chunk (blb_s, blbt,  etc)

#include <stdio.h>
#include <string.h>

#include "config.h"

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

int DmxClass::_blbt_index(unsigned char *h)
{
  for (int i = 0; i < this->blbt_cnt; i++) {
    if (!memcmp(h, this->blbt[i].h, HASH_LEN))
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

void DmxClass::arm_blb(unsigned char *h,
             void (*fct)(unsigned char*, int, int, struct face_s*),
                       unsigned char *fid, int seq, int cnr, int last)
{
  int ndx = _blbt_index(h);
  struct blb_s *bptr = ndx < 0 ? NULL : blbt + ndx;
  if (fct == NULL) {
    // Serial.printf("arm_blb REMOVE\r\n");
    if (bptr != NULL) {
      while (bptr->front) {
        struct chain_s *tp = bptr->front->next;
        free(bptr->front);
        bptr->front = tp;
      }
      memmove(bptr, bptr+1, (blbt_cnt - ndx - 1) * sizeof(struct blb_s));
      blbt_cnt--;
    }
    return;
  }
  if (bptr == NULL) {
    ndx = this->blbt_cnt++;
    bptr = blbt + ndx;
    memset(bptr, 0, sizeof(struct blb_s));
  }

  /*
  // FIXME: could simply below because we always remove the whole chain
  // and we never add the same tuple twice
  if (ndx < 0 && fct != NULL) {
    if (blbt_cnt >= BLBT_SIZE) {
      Serial.println("arm_dmx: blbt is full");
      return; // full
    }
    ndx = this->blbt_cnt++;
    memset(blbt+ndx, 0, sizeof(struct blb_s));
  }
  if (ndx < 0)
    return;
  struct blb_s *bptr = blbt + ndx;
  struct chain_s *t = bptr->front, **tp = &(bptr->front);
  while (t) { // remove (fid,seq,cnr) tuples from the linked list
    Serial.printf(" wloop %p\r\n", t);
    if (fct == NULL ||
        (!memcmp(t->fid, fid, FID_LEN) && t->seq == seq && t->cnr == cnr) ) {
      *tp = t->next;
      free(t);
      t = *tp;
    } else {
      tp = &(t->next);
      t = t->next;
    }
  }
  if (fct == NULL) {
    if (bptr->front == NULL) { // remove if linked list empty
      Serial.printf(" remove if empty\r\n");
      memmove(blbt+ndx, blbt+ndx+1,
              (blbt_cnt - ndx - 1) * sizeof(struct blb_s));
      blbt_cnt--;
    } else
      Serial.printf(" remove from blbt: front not empty\r\n");
    return;
  }
  */
  memcpy(bptr->h, h, HASH_LEN);
  bptr->fct = fct;
  struct chain_s *tp = (struct chain_s*) malloc(sizeof(struct chain_s));
  tp->fid = fid; // (unsigned char*) malloc(FID_LEN);
  tp->seq = seq;
  tp->cnr = cnr;
  tp->last_cnr = last;
  tp->next = bptr->front;
  bptr->front = tp;
  /*
  int ndx = this->_blbt_index(h);
  // Serial.printf(" _ arm_blb ndx=%d %s\r\n", ndx, to_hex(h, HASH_LEN));
  if (ndx >= 0) { // this entry will be either erased or newly written to
    // Serial.printf("   %p\r\n", this->blbt[ndx].fid);
    // free(this->blbt[ndx].fid);
    // int fNDX = theGOset->_key_index(this->blbt[ndx].fid);
    // Serial.printf(" _ squashing old CHKTAB entry %d %s %d.%d.%d\r\n",
    //               ndx, to_hex(h, HASH_LEN), fNDX,
    //               this->blbt[ndx].seq, this->blbt[ndx].bnr);
  }
  if (fct == NULL) { // del
    if (ndx >= 0) {
      // Serial.printf("   ->%p\r\n", this->blbt[ndx].fid);
      memmove(this->blbt+ndx, this->blbt+ndx+1,
              (this->blbt_cnt - ndx - 1) * sizeof(struct blb_s));
      // int fNDX = theGOset->_key_index(this->blbt[ndx].fid);
      // Serial.printf(" _ del CHKTAB entry %d %s %d.%d.%d\r\n", ndx, to_hex(h, HASH_LEN), fNDX,
      //               this->blbt[ndx].seq, this->blbt[ndx].bnr);
      this->blbt_cnt--;
    }
    return;
  }
  if (ndx == -1) {
    if (this->blbt_cnt >= BLBT_SIZE) {
      Serial.println("adm_dmx: blbt is full");
      return; // full
    }
    ndx = this->blbt_cnt++;
  }
  // int fNDX = theGOset->_key_index(fid);
  // Serial.printf(" _ new CHKTAB entry @%d %s %d.%d.%d/%d\r\n", ndx,
  //               to_hex(h, HASH_LEN), fNDX, seq, bnr, last);
  memcpy(this->blbt[ndx].h, h, HASH_LEN);
  this->blbt[ndx].fct = fct;
  this->blbt[ndx].fid = fid; // (unsigned char*) malloc(FID_LEN);
  // memcpy(this->blbt[ndx].fid, fid, FID_LEN);
  this->blbt[ndx].seq = seq;
  this->blbt[ndx].bnr = bnr;
  this->blbt[ndx].last_bnr = last;
  */
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
  ndx = this->_blbt_index(h);
  if (ndx >= 0) {
    this->blbt[ndx].fct(buf, len, ndx, f);
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
