// replica.h

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

// persistency for a single feed, "1FPF" (one file per feed)

#ifndef _INCLUDE_REPLICA_H
#define _INCLUDE_REPLICA_H

#include "tinySSBlib.h"


#define PFX "tinyssb-v0"
#define PFX_LEN 10
void compute_dmx(unsigned char *dst,
                 unsigned char *fid, int seq, unsigned char *prev);

struct chunk_needed_s {
  unsigned short snr, cnr;
  unsigned char hash[HASH_LEN];
};


class ReplicaClass {

public:
  ReplicaClass(char *datapath, unsigned char *fid);
  int load_chunk_cnt(); // not included in init because of scan time

  char ingest_entry_pkt(unsigned char *pkt); // True/False
  char ingest_chunk_pkt(unsigned char *pkt, int seq, int *next_cnr); // True/False
  int get_next_seq(unsigned char *dmx = NULL); // returns seq and DMX
  unsigned char* get_entry_pkt(int seq);
  unsigned char* get_chunk_pkt(int seq, int chunk_nr);
  int get_open_sidechains(int max_cnt, struct chunk_needed_s *pcn);
  // struct bipf_s* get_open_chains();
  // struct bipf_s* get_next_in_chain(int seq);
  void hex_dump(int seq);

  // the following fcts are not needed for mere forwarding repos (pubs)
  int get_mid(int seq, unsigned char *mid); // returns message ID
  int get_content_len(int seq, int *valid_len = NULL);
  unsigned char* read(int seq, int *valid_len = NULL);
  //  write48(self, content, sign_fct): # publish event, returns seq or None
  //  write(self, content, sign_fct): # publish event, returns seq or None

  unsigned char fid[FID_LEN];
  int chunk_cnt = -1;
private:
  char *fname;

  uint32_t seq = 0;            // highest stored seq nr
  unsigned char mid[HASH_LEN]; // mid of above packet
  uint32_t pendscc = 0;        // pending side chain chain (as stored), or 0
  uint32_t pssc_iter = 0;      // last seq nr we looked at for open side chain

  // struct bipf_s *state; // NULL if not loaded yet
  // struct bipf_s *max_seq_ref, *max_pos_ref, *prev_ref, *pending_ref;

  // void _load_state();
  int _get_content_len(unsigned char *pkt, int seq, int *valid_len = NULL);
  File _open_at_start(int seq, uint32_t *endAddr = NULL);
  // void _persist_frontier();
  void _mk_fname(int n);
  // void _init_frontier();
};

#endif // _INCLUDE_REPLICA_H

// eof
