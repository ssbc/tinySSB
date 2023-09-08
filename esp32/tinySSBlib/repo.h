// repo.h

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

#ifndef _INCLUDE_REPO2_H
#define _INCLUDE_REPO2_H

#include "tinySSBlib.h"


class Repo2Class {

public:
  void           clean(char *path); // recursively rm files IN this dir
  void           reset(char *path); // as above, remove the dir and reboot
  void           load();
  void           add_replica(unsigned char *fid);

  ReplicaClass*  fid2replica(unsigned char* fid);

  void mk_want_vect();
  void mk_chnk_vect();

  ReplicaClass*  replicas[MAX_FEEDS];
  int rplca_cnt;
  int entry_cnt;
  int chunk_cnt;

  unsigned short want_offs, chnk_offs;
  unsigned char *want_vect, *chnk_vect;
  unsigned char want_len, chnk_len;
  unsigned char want_is_valid, chnk_is_valid;
};

#endif // _INCLUDE_REPO2_H

// eof
