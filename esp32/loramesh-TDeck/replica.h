// replica.h

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

// persistency for a single feed, using "2FPF" (two files per feed)

#if !defined(_INCLUDE_REPLICA_H)
#define _INCLUDE_REPLICA_H


#define PFX "tinyssb-v0"
#define PFX_LEN 10
void compute_dmx(unsigned char *dst,
                 unsigned char *fid, int seq, unsigned char *prev);

class ReplicaClass {

public:
  ReplicaClass(char *datapath, unsigned char *fid);

  char ingest_entry_pkt(unsigned char *pkt); // True/False
  char ingest_chunk_pkt(unsigned char *pkt, int seq, int *next_cnr); // True/False
  int get_next_seq(unsigned char *dmx = NULL); // returns seq and DMX
  int get_content_len(int seq, int *valid_len = NULL);
  int get_chunk_cnt();
  unsigned char* get_prev();
  struct bipf_s* get_open_chains();
  struct bipf_s* get_next_in_chain(int seq);
  unsigned char* get_entry_pkt(int seq);
  unsigned char* get_chunk_pkt(int seq, int chunk_nr);
  void hex_dump(int seq);

  // the following fcts are not needed for mere forwarding repos (pubs)
  unsigned char* read(int seq, int *valid_len = NULL);
  //  write48(self, content, sign_fct): # publish event, returns seq or None
  //  write(self, content, sign_fct): # publish event, returns seq or None

  unsigned char fid[FID_LEN];
  
private:
  char *fname;
  int suffix_pos;
  struct bipf_s *state, *max_seq_ref, *max_pos_ref, *prev_ref, *pending_ref;

  int _get_content_len(unsigned char *pkt, int seq, int *valid_len = NULL);
  File _get_entry_start(int seq);
  void _persist_frontier();
  void _mk_fname(int n);
  void _init_frontier();
};

#endif // _INCLUDE_REPLICA_H

// eof
