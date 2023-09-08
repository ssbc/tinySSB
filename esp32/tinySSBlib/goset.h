// goset.h

// tinySSB for ESP32
// Aug 2022 <christian.tschudin@unibas.ch>

// Grow-Only Set

#ifndef _INCLUDE_GOSET_H
#define _INCLUDE_GOSET_H

#include "tinySSBlib.h"


#define GOSET_KEY_LEN FID_LEN // 32
#define GOSET_MAX_KEYS      MAX_FEEDS
#define GOSET_ROUND_LEN  20000 // in millis
#define MAX_PENDING        10 // log2(maxSetSize) + density (#neighbors)
#define NOVELTY_PER_ROUND   1
#define ASK_PER_ROUND       1
#define HELP_PER_ROUND      2
#define ZAP_ROUND_LEN    4500


struct claim_s {
  unsigned char typ;
  unsigned char lo[GOSET_KEY_LEN];
  unsigned char hi[GOSET_KEY_LEN];
  unsigned char xo[GOSET_KEY_LEN];
  unsigned char cnt;
};

struct novelty_s {
  unsigned char typ;
  unsigned char key[GOSET_KEY_LEN];
};

struct zap_s {
  unsigned char typ;
  unsigned char key[GOSET_KEY_LEN];
  int32_t ndx; // -1: zap everything, else: index of entry to zap
};


class GOsetClass {
 public:
  int goset_len; // number of set elements
  unsigned char goset_state[GOSET_KEY_LEN];
  int pending_c_cnt, pending_n_cnt;

  void dump();
  void add(unsigned char *key);
  void populate(unsigned char *key); // just insert, or sort when key is NULL
  void rx(unsigned char *pkt, int len, unsigned char *aux, struct face_s *f);
  void tick();
  void do_zap(int ndx);
  int _key_index(unsigned char *key);
  unsigned char *get_key(int ndx);

  void probe_for_goset_vect(unsigned char **pkt,
                            unsigned short *len,
                            unsigned short *reprobe_in_millis);
  
 private:
  unsigned long last_round;
  unsigned char goset_keys[GOSET_KEY_LEN * GOSET_MAX_KEYS];
  int largest_claim_span;
  struct claim_s pending_claims[MAX_PENDING];
  struct novelty_s pending_novelty[MAX_PENDING];
  int novelty_credit;
  unsigned long next_round; // FIXME: handle, or test, wraparound
  struct zap_s zap;
  unsigned long zap_state;
  unsigned long zap_next;

  unsigned char* _xor(int lo, int hi);
  int isZero(unsigned char *h, int len);
  unsigned char* _mkClaim(int lo, int hi);
  unsigned char* _mkNovelty(unsigned char *start, int cnt);
  unsigned char* _mkZap();
  void _add_pending(unsigned char *claim);
};

/* packet format:

  n 32B 32B? 32B?  // 33 bytes, in the future up to two additional keys
  c 32B 32B 32B B  // 98 bytes
    lo  hi  xor cnt
  z 32B(nonce)     // 33 bytes
*/

// forward declaration
extern void set_want_dmx();

// -------------------------------------------------------------------------

// struct goset_s *theGOset;

#define NOVELTY_LEN sizeof(struct novelty_s)
#define CLAIM_LEN   sizeof(struct claim_s)
#define ZAP_LEN     sizeof(struct zap_s)

#endif

// eof
