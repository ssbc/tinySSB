// repo.h

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

#ifndef _INCLUDE_REPO2_H
#define _INCLUDE_REPO2_H

#include "tinySSBlib.h"

struct feedRecord_s { // make it a 2^n record to avoid block crossing
  unsigned char fid[FID_LEN];
  uint32_t fn;  /* uint32_t file name --> hex --> 8 characters, ending "aol"
                   we take the first 4B of the FID. In case of a collision
                   the "file name number" is incremented as necessary */
  unsigned char mid[HASH_LEN];
  unsigned char seq[2];           // network order
  unsigned char logsize[3];       // network order
  unsigned char pendscc[3];       // network order
};

/* "generous" and extended version:

  32B fid
  12B filename
  20B mid
   4B seq (=entry_cnt)
   4B chunk_cnt
   4B offs (for later when logs don't start at 0)
   4B logsize (file could be longer in case of crash before writing this file)
   4B pendscc
  ---
  84B total
  44B padding -> 128B/feed

*/
#assert sizeof(struct feedRecord_s) == 64


class Repo2Class {

public:
  void           clean(char *path); // recursively rm files IN this dir
  void           reset(char *path); // as above, remove the dir and reboot
  void           load();
  void           create_replica(unsigned char *fid);
  void           loop(); // idle loop, e.g. for indexing

  Replica3Class* fid2replica(unsigned char* fid);
  void           update_feed_rec(Replica3Class *r);

  void mk_want_vect();
  void mk_chnk_vect();

  Replica3Class* replicas[MAX_FEEDS];
  int rplca_cnt = 0;
  int entry_cnt = 0;
  int chunk_cnt = 0;

  unsigned short want_offs = 0, chnk_offs = 0;
  unsigned char *want_vect, *chnk_vect;
  unsigned char want_len, chnk_len;
  unsigned char want_is_valid = false, chnk_is_valid = false;
private:
  struct feedRecord_s *feeds;
};

#endif // _INCLUDE_REPO2_H

// eof
