// replica.cpp

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

// persistency for a single feed, using "2FPF" (two files per feed)

/*
  feeds are stored as two files:
    'log.bin'      the log, with side chain expanded right after a log entry
    'frt.bin'      feed state, in BIPF
    'tmp.bin'      same as above, before being renamed to frt.bin
*/


#include <lwip/def.h>
#include <LittleFS.h>
#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_sign_ed25519.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#define MyFS LittleFS

static unsigned char nam[PFX_LEN + FID_LEN + 4 + HASH_LEN + TINYSSB_PKT_LEN];  // name computation

void compute_dmx(unsigned char *dst,
                 unsigned char *fid, int seq, unsigned char *prev)
{
  memcpy(nam, PFX, PFX_LEN); // FIXME: do this once in repo.cpp
  memcpy(nam + PFX_LEN, fid, FID_LEN);
  unsigned int *iptr = (unsigned int*) (nam + PFX_LEN + FID_LEN);
  *iptr = htonl(seq);
  memcpy(nam + PFX_LEN + FID_LEN + 4, prev, HASH_LEN);
  unsigned char h[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(h, nam, sizeof(nam) - TINYSSB_PKT_LEN);
  memcpy(dst, h, 7);
}

ReplicaClass::ReplicaClass(char *datapath, unsigned char *fID)
{
  memcpy(this->fid, fID, FID_LEN);

  fname = (char*) malloc(strlen(datapath) + 1 + 64 + 1 + 7 + 1); // "/log.bin"
  sprintf(fname, "%s/%s", FEED_DIR, to_hex(fid, FID_LEN, 0));
  if (!MyFS.exists(fname))
    MyFS.mkdir(fname);
  strcat(fname, "/");
  suffix_pos = strlen(fname);

  _mk_fname(0); // log
  if (!MyFS.exists(fname)) { // create log and frt
    MyFS.open(fname, "wb").close();
    // init the frt.bin file:
    _init_frontier();
  } else {
    File f = MyFS.open(fname, "rb");
    int log_len = f.size();
    f.close();
    // check whether tmp.bin exists, do the rename op that was lost
    _mk_fname(2);
    if (MyFS.exists(fname)) {
      char *old = strdup(fname);
      _mk_fname(1);
      MyFS.rename(old, fname);
    }
    // check whether frt.bin exists, else init one
    _mk_fname(1);
    if (!MyFS.exists(fname))
      _init_frontier();
    // read the frontier file
    _mk_fname(1);
    f = MyFS.open(fname);
    int frt_len = f.size();
    unsigned char *buf = (unsigned char*) malloc(frt_len);
    f.read(buf, frt_len);
    f.close();
    // Serial.printf("   frt content is %s\r\n", to_hex(buf, frt_len, 0));
    state = bipf_loads(buf, frt_len);
    // Serial.printf("FID=%s\r\n", to_hex(fID, 32));
    // Serial.println("   state=" + bipf2String(state));
    free(buf);
    max_seq_ref = bipf_dict_getref(state, str2bipf("max_seq"));
    max_pos_ref = bipf_dict_getref(state, str2bipf("max_pos"));
    prev_ref    = bipf_dict_getref(state, str2bipf("prev"));
    pending_ref = bipf_dict_getref(state, str2bipf("pend_sc"));
    // Serial.printf("   msr=%p mpr=%p pvr=%p per=%p\r\n",
    //               max_seq_ref, max_pos_ref, prev_ref, pending_ref);
    // Serial.println("   pend_sc=" + bipf2String(pending_ref));

    // check whether frt is up to date
    // ... FIXME else redigest the log entries, then _persist_frontier
  /*
        while os.path.getsize(self.log_fname) > self.state['max_pos']:
            with open(self.log_fname, 'r+b') as f:
                pos = self.state['max_pos']
                f.seek(pos, os.SEEK_SET)
                pkt = f.read(120)
                seq = self.state['max_seq'] + 1
                nam = PFX + self.fid + seq.to_bytes(4,'big') + self.state['prev']
                dmx = hashlib.sha256(nam).digest()[:7]
                if self.is_author or dmx != pkt[:7]:
                    print('truncating log file')
                    f.seek(pos, os.SEEK_SET)
                    f.truncate()
                    break
                chunk_cnt = 0
                if pkt[7] == PKTTYPE_chain20:
                    content_len, sz = bipf.varint_decode(pkt, 8)
                    content_len -= 48 - 20 - sz
                    ptr = pkt[36:56]
                    chunk_cnt = (content_len + 99) // 100
                if chunk_cnt > 0:
                    self.state['pend_sc'][seq] = [0, chunk_cnt, ptr, pos + 120]
                while chunk_cnt > 0: # allocate sidechain space in the file
                    f.write(bytes(120))
                    chunk_cnt -= 1
                f.write(pos.to_bytes(4,'big'))
                pos = f.tell()
            self._persist_frontier(seq, pos,
                                   hashlib.sha256(nam + pkt).digest()[:20])
  */
  }
}

void ReplicaClass::_persist_frontier()
{
  int len;
  unsigned char *buf = bipf_dumps(state, &len);
  // Serial.printf("   _persist_frontier %dB %s\r\n", len, to_hex(buf, len, 0));
  _mk_fname(2); // tmp
  char *tmp = strdup(fname);
  // Serial.printf("open\r\n");
  File f = MyFS.open(tmp, "wb");
  f.write(buf, len);
  f.close();
  // Serial.printf("closed\r\n");
  free(buf);
  _mk_fname(1); // frt
  MyFS.rename(tmp, fname);
  free(tmp);
  // Serial.printf("renamed\r\n");
}

void ReplicaClass::_mk_fname(int n)
{
  if (n == 2)
    strcpy(fname + suffix_pos, "tmp.bin");
  else if (n == 1)
    strcpy(fname + suffix_pos, "frt.bin");
  else
    strcpy(fname + suffix_pos, "log.bin");
}

void ReplicaClass::_init_frontier()
{
  state = bipf_mkDict();
  max_seq_ref = bipf_mkInt(0);
  bipf_dict_set(state, bipf_mkString("max_seq"), max_seq_ref);
  max_pos_ref = bipf_mkInt(0);
  bipf_dict_set(state, bipf_mkString("max_pos"), max_pos_ref);
  prev_ref = bipf_mkBytes(fid, HASH_LEN);
  bipf_dict_set(state, bipf_mkString("prev"), prev_ref);
  pending_ref = bipf_mkDict();
  bipf_dict_set(state, bipf_mkString("pend_sc"), pending_ref);
  _persist_frontier();
}

char ReplicaClass::ingest_entry_pkt(unsigned char *pkt) // True/False
{
  // Serial.println(String("incoming entry for log ") + to_hex(fid, FID_LEN, 0));
  /*
  long durations[10], t1, t2;
  t1 = millis();

  int ndx = feed_index(fid);
  if (ndx < 0) {
    Serial.println("  no such feed");
    return;
  }
  t2 = millis(); durations[0] = t2 - t1; t1 = t2;
  */
  // check dmx
  unsigned char dmx_val[DMX_LEN];
  compute_dmx(dmx_val, fid, max_seq_ref->u.i + 1, prev_ref->u.buf);
  if (memcmp(dmx_val, pkt, DMX_LEN)) { // wrong dmx field
    Serial.println("   DMX mismatch");
    return 0;
  }
  fishForNewLoRaPkt();

  // check signature, nam still contains the packet's name
  memcpy(nam + strlen(DMX_PFX) + FID_LEN + 4 + HASH_LEN, pkt, TINYSSB_PKT_LEN);
  int b = crypto_sign_ed25519_verify_detached(pkt + 56, nam, PFX_LEN + FID_LEN + 4 + HASH_LEN + 56, fid);
  if (b) {
    Serial.println("   ed25519 signature verification failed");
    return 0;
  }
  unsigned char h256[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(h256, nam, sizeof(nam));
  memcpy(prev_ref->u.buf, h256, HASH_LEN); // =msgID
  // t2 = millis(); durations[2] = t2 - t1; t1 = t2;
  fishForNewLoRaPkt();

  _mk_fname(0); // log
  File f = MyFS.open(fname, "r+b" /*FILE_APPEND*/);
  f.write(pkt, TINYSSB_PKT_LEN);
  // Serial.printf("   appended %d.%d\r\n", theGOset->_key_index(fid),
  //               max_seq_ref->u.i + 1);
  int chunk_cnt = 0;
  if (pkt[DMX_LEN] == PKTTYPE_chain20) {
    int sz = 5;
    int content_len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);
    content_len -= 48 - HASH_LEN - sz;
    // ptr = pkt[36:56] --> pending_sc
    chunk_cnt = (content_len + 99) / 100;
    // Serial.printf("  content_len=%d, cc=%d\r\n", content_len, chunk_cnt);
  }
  if (chunk_cnt > 0) {
    struct bipf_s *sc = bipf_mkList();
    bipf_list_append(sc, bipf_mkInt(0));
    bipf_list_append(sc, bipf_mkInt(chunk_cnt));
    bipf_list_append(sc, bipf_mkBytes(pkt+36, HASH_LEN));
    bipf_list_append(sc, bipf_mkInt(max_pos_ref->u.i + TINYSSB_PKT_LEN));
    /*
    Serial.printf("* adding to sidechain: %d/%d pos=%d, dict_len=%d\r\n",
                  max_seq_ref->u.i + 1, chunk_cnt,
                  max_pos_ref->u.i + TINYSSB_PKT_LEN,
                  pending_ref->cnt);
    */
    bipf_dict_set(pending_ref, bipf_mkInt(max_seq_ref->u.i + 1), sc);
    // Serial.printf("  dict_len=%d\r\n", pending_ref->cnt);
    unsigned char empty[TINYSSB_PKT_LEN];
    memset(empty, 0, sizeof(empty));
    for (int i = 0; i < chunk_cnt; i++)
      f.write(empty, sizeof(empty));
  }
  uint oldpos = htonl(max_pos_ref->u.i);
  f.write((unsigned char *) &oldpos, sizeof(oldpos));
  f.close();
  max_pos_ref->u.i += (chunk_cnt + 1) * TINYSSB_PKT_LEN + sizeof(oldpos);
  max_seq_ref->u.i++;
  _persist_frontier();
  // t2 = millis(); durations[4] = t2 - t1; t1 = t2;
  fishForNewLoRaPkt();

  // t2 = millis(); durations[5] = t2 - t1; t1 = t2;
  // t2 = millis(); durations[6] = t2 - t1; t1 = t2;

  /*
  Serial.printf("   durations");
  for (int i = 0; i < sizeof(durations)/sizeof(long); i++)
    Serial.printf(" %ld", durations[i]);
  Serial.printf("\r\n");
  */
  // Serial.printf("end of ingest_entry_pkt\r\n");
  return 1;
}

char ReplicaClass::ingest_chunk_pkt(unsigned char *pkt, int seq, int *cnr) // True/False
{
  unsigned char h[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(h, pkt, TINYSSB_PKT_LEN);
  struct bipf_s k = { BIPF_INT, {}, {.i = seq} };
  struct bipf_s *pend = bipf_dict_getref(pending_ref, &k);
  // [cnr, rem, hptr, pos]
  // assert correct hash val
  if (pend == NULL || memcmp(h, pend->u.list[2]->u.buf, HASH_LEN)) {
    // Serial.printf("unexpected chunk for seq=%d\r\n", seq);
    return 0;
  }
  _mk_fname(0); // log
  File f = MyFS.open(fname, "r+b");
  if (!f)
    return 0;
  int pos = pend->u.list[3]->u.i;
  f.seek(pos, SeekSet);
  f.write(pkt, TINYSSB_PKT_LEN);
  f.close();
  // Serial.printf("chunk saved to disk\r\n");
  if (pend->u.list[1]->u.i <= 1) { // chain is complete
    bipf_dict_delete(pending_ref, &k);
    if (cnr)
      *cnr = -1;
  } else {
    pend->u.list[0]->u.i++;
    pend->u.list[1]->u.i--;
    memcpy(pend->u.list[2]->u.buf, pkt+100, HASH_LEN);
    pend->u.list[3]->u.i = pos + TINYSSB_PKT_LEN;
    if (cnr)
      *cnr = pend->u.list[0]->u.i;
  }
  _persist_frontier();
  // Serial.printf("end of ingest_chunk_pkt\r\n");
  return 1;
}

int ReplicaClass::get_next_seq(unsigned char *dmx) // returns seq and DMX
{
  // Serial.printf(" max_seq_ref=%p prev_ref=%p\r\n", max_seq_ref, prev_ref);
  int maxs = max_seq_ref->u.i;
  // Serial.printf(" max_seq=%d\r\n", maxs);
  // Serial.printf(" prev=%s\r\n", to_hex(prev_ref->u.buf, prev_ref->cnt, 0));
  if (dmx)
    compute_dmx(dmx, fid, maxs + 1, prev_ref->u.buf);
  return maxs + 1;
}

int ReplicaClass::get_content_len(int seq)
{
  // .. FIXME also return # of available bytes, not needed for pub?
  unsigned char *pkt = this->get_entry_pkt(seq);
  if (!pkt)
    return -1;
  if (pkt[DMX_LEN] == PKTTYPE_plain48)
    return 48; // (48,48)
  if (pkt[DMX_LEN] != PKTTYPE_chain20)
    return -1;
  int sz = 5;
  int content_len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);
  return content_len;
  /*
            if not seq in self.state['pend_sc']:
                return (content_len, content_len)
            available = (48-20-sz) + 100 * self.state['pend_sc'][seq][0]
            return (available, content_len)
  */
}

int ReplicaClass::get_chunk_cnt()
{
  int c = max_pos_ref->u.i - (TINYSSB_PKT_LEN + 4) * max_seq_ref->u.i;
  c /= TINYSSB_PKT_LEN;
  for (int i = 0; i < pending_ref->cnt; i++) {
    struct bipf_s *s = pending_ref->u.dict[2*i+1];
    c -= s->u.list[1]->u.i; // remaining
  }
  return c;
}

struct bipf_s* ReplicaClass::get_open_chains() // return a dict
{
  return pending_ref;
}

struct bipf_s* ReplicaClass::get_next_in_chain(int seq)
{
  struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
  return bipf_dict_getref(pending_ref, &i);
}

unsigned char* ReplicaClass::get_entry_pkt(int seq)
{
  if (seq < 1 || seq > max_seq_ref->u.i)
    return NULL;
  static unsigned char buf[TINYSSB_PKT_LEN];
  File f = MyFS.open(fname, "rb");
  if (!f)
    return NULL;
  uint pos = f.size();
  int cnt = max_seq_ref->u.i - seq + 1;
  while (cnt-- > 0) {
    f.seek(pos-4, SeekSet);
    if (f.read( (unsigned char*) &pos, sizeof(pos) ) != sizeof(pos)) {
      f.close();
      return NULL;
    }
  }
  f.seek(pos, SeekSet);
  int sz = f.read(buf, sizeof(buf));
  f.close();
  return sz == sizeof(buf) ? buf : NULL;
}

unsigned char* ReplicaClass::get_chunk_pkt(int seq, int chunk_nr)
{
  if (seq < 1 || seq > max_seq_ref->u.i)
    return NULL;
  // check that we persisted this chunk, is not in a pending sidechain
  struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
  struct bipf_s *p = bipf_dict_getref(pending_ref, &i);
  if (p != NULL && chunk_nr >= p->u.list[0]->u.i)
    return NULL;
  // read the chunk
  static unsigned char buf[TINYSSB_PKT_LEN];
  File f = MyFS.open(fname, "rb");
  if (!f)
    return NULL;
  uint pos = f.size();
  int cnt = max_seq_ref->u.i - seq + 1;
  int lim;
  while (cnt-- > 0) {
    f.seek(pos-4, SeekSet);
    lim = pos;
    if (f.read( (unsigned char*) &pos, sizeof(pos) ) != sizeof(pos)) {
      f.close();
      return NULL;
    }
    pos = ntohl(pos);
  }
  pos += TINYSSB_PKT_LEN * (chunk_nr+1);
  if (pos > lim-TINYSSB_PKT_LEN) {
    f.close();
    return NULL;
  }
  f.seek(pos, SeekSet);
  int sz = f.read(buf, sizeof(buf));
  f.close();
  return sz == sizeof(buf) ? buf : NULL;
}

// eof
