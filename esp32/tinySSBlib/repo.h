// repo.h

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

#ifndef _INCLUDE_REPO2_H
#define _INCLUDE_REPO2_H

#include "tinySSBlib.h"


class Repo2Class {

public:
  Repo2Class(is_complete_fct cb_log_novelty = NULL);

  void           clean(char *path); // recursively rm files IN this dir
  void           reset(char *path); // as above, remove the dir and reboot
  void           load();
  void           add_replica(unsigned char *fid);
  void           loop(); // idle loop, e.g. for indexing

  ReplicaClass*  fid2replica(unsigned char* fid);

  void mk_want_vect();
  void mk_chnk_vect();

  ReplicaClass* replicas[MAX_FEEDS];
  int rplca_cnt = 0;
  int entry_cnt = 0;
  int chunk_cnt = 0;

  unsigned short want_offs = 0, chnk_offs = 0;
  unsigned char *want_vect = NULL, *chnk_vect = NULL;
  unsigned char want_len, chnk_len;
  unsigned char want_is_valid = false, chnk_is_valid = false;

  is_complete_fct cb_completed; // cb_log_novelty;
};


#endif // _INCLUDE_REPO2_H

// eof
