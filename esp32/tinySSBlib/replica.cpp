// replica.cpp

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

// persistency for a single feed, using "2FPF" (two files per feed)

/*
  feeds are stored as two files:
    'log.bin'      the log, with side chain expanded right after a log entry
    'frt.bin'      feed state, in BIPF

    'tmp.bin'      same as above, before being renamed to frt.bin

    for nodes with apps:
    'mid.bin'      sequence of 20B message IDs (back to back)
*/

#include "tinySSBlib.h"


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

#ifdef ERASE
  // erase:
  _mk_fname(0); // log
  MyFS.open(fname, FILE_WRITE, true).close();
  _mk_fname(1); // frt
  MyFS.remove(fname);
#endif

  _mk_fname(0); // log
  if (!MyFS.exists(fname)) { // create log and frt
    MyFS.open(fname, FILE_WRITE, true).close();
    _mk_fname(3); // mid
    if (!MyFS.exists(fname))
      MyFS.open(fname, FILE_WRITE, true).close();
    // init the frt.bin file:
    _init_frontier();
  } else {
    File f = MyFS.open(fname, FILE_READ, false);
    int log_len = f.size();
    f.close();
    // check whether tmp.bin exists, do the rename op that was lost
    _mk_fname(2);
    if (MyFS.exists(fname)) {
      char *old = strdup(fname);
      _mk_fname(1);
      MyFS.rename(old, fname);
    }
 
    // FIXME: fill MID content, if necessary
    
    // check whether frt.bin exists, else init one
    _mk_fname(1);
    if (!MyFS.exists(fname))
      _init_frontier();
    else {
      state = NULL; // delay state loading
#ifdef NOT_USED
      // read the frontier file
      _mk_fname(1);
      f = MyFS.open(fname, FILE_READ, false);
      int frt_len = f.size();
      // FIXME: check whether length is zero, recover content
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
 #endif
    }
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


void ReplicaClass::_load_state()
{
  // Serial.println("   _load_state");
  if (state != NULL)
    return;
  // read the frontier file
  _mk_fname(1);
  File f = MyFS.open(fname, FILE_READ, false);
  int frt_len = f.size();
  // FIXME: check whether length is zero, recover content
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
}


int ReplicaClass::_get_content_len(unsigned char *pkt, int seq, int *valid_len)
{
  if (pkt[DMX_LEN] == PKTTYPE_plain48) {
    if (valid_len)
      *valid_len = 48;
    return 48;
  }
  int sz = 5;
  int len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);
  if (valid_len) {
    *valid_len = len;
    if (len > 48-20-sz) {
      _load_state();
      struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
      struct bipf_s *p = bipf_dict_getref(pending_ref, &i);
      if (p != NULL)
        *valid_len = (48-20-sz) + 100 * p->u.list[0]->u.i;
    }
  }
  return len;
  /*
            if not seq in self.state['pend_sc']:
                return (content_len, content_len)
            available = (48-20-sz) + 100 * self.state['pend_sc'][seq][0]
            return (available, content_len)
  */
}


File ReplicaClass::_get_entry_start(int seq)
{
  _load_state();
  if (seq < 1 || seq > max_seq_ref->u.i)
    return (File) NULL;
  _mk_fname(0); // log
  File f = MyFS.open(fname, FILE_READ, false);
  if (!f)
    return (File) NULL;
  uint pos = f.size();
  int cnt = max_seq_ref->u.i - seq + 1;
  while (cnt-- > 0) {
    // Serial.printf("pos=%d\r\n", pos);
    f.seek(pos-4, SeekSet);
    if (f.read( (unsigned char*) &pos, sizeof(pos) ) != sizeof(pos)) {
      // Serial.printf("didn't worked pos=%d, cnt=%d\r\n", ntohl(pos), cnt);
      f.close();
      return (File) NULL;
    }
    pos = ntohl(pos);
  }
  // Serial.printf("worked %d\r\n", pos);
  f.seek(pos, SeekSet);
  return f;
}


void ReplicaClass::_persist_frontier()
{
  int len;
  unsigned char *buf = bipf_dumps(state, &len);
  // Serial.printf("   _persist_frontier %dB %s\r\n", len, to_hex(buf, len, 0));
  _mk_fname(2); // tmp
  char *tmp = strdup(fname);
  // Serial.printf("open\r\n");
  File f = MyFS.open(tmp, FILE_WRITE, true);
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
  if (n == 3)
    strcpy(fname + suffix_pos, "mid.bin");
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
  _load_state();  
  unsigned char dmx_val[DMX_LEN];
  compute_dmx(dmx_val, fid, max_seq_ref->u.i + 1, prev_ref->u.buf);
  if (memcmp(dmx_val, pkt, DMX_LEN)) { // wrong dmx field
    Serial.println("   DMX mismatch");
    return 0;
  }
  io_loop();

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
  io_loop();

  _mk_fname(0); // log
  File f = MyFS.open(fname, FILE_APPEND, false);
  // Serial.printf("   appending %d.%d at %d\r\n", theGOset->_key_index(fid),
  //                max_seq_ref->u.i + 1, f.position());
  f.write(pkt, TINYSSB_PKT_LEN);
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
    // Serial.printf("  1st chunk will be at %d\r\n", f.position());
    struct bipf_s *sc = bipf_mkList();
    bipf_list_append(sc, bipf_mkInt(0));
    bipf_list_append(sc, bipf_mkInt(chunk_cnt));
    bipf_list_append(sc, bipf_mkBytes(pkt+36, HASH_LEN));
    bipf_list_append(sc, bipf_mkInt(f.position())); // max_pos_ref->u.i + TINYSSB_PKT_LEN));
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
  io_loop();

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
  _load_state();

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
  File f = MyFS.open(fname, "r+", false);
  if (!f)
    return 0;
  unsigned long pos = pend->u.list[3]->u.i;
  // Serial.printf("  saving chunk %d.%d at %d\r\n", seq, *cnr, pos);
  f.seek(pos, SeekSet);
  // Serial.printf("  pos after seek is %d\r\n", f.position());
  f.write(pkt, TINYSSB_PKT_LEN);
  // Serial.printf("  pos after write is %d\r\n", f.position());
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
  _load_state();

  // Serial.printf(" max_seq_ref=%p prev_ref=%p\r\n", max_seq_ref, prev_ref);
  int maxs = max_seq_ref->u.i;
  // Serial.printf(" max_seq=%d\r\n", maxs);
  // Serial.printf(" prev=%s\r\n", to_hex(prev_ref->u.buf, prev_ref->cnt, 0));
  if (dmx)
    compute_dmx(dmx, fid, maxs + 1, prev_ref->u.buf);
  return maxs + 1;
}

int ReplicaClass::get_content_len(int seq, int *valid_len)
{
  unsigned char *pkt = this->get_entry_pkt(seq);
  if (!pkt)
    return -1;
  if (pkt[DMX_LEN] != PKTTYPE_plain48 && pkt[DMX_LEN] != PKTTYPE_chain20)
    return -1;
  return _get_content_len(pkt, seq, valid_len);
  /*
  if (pkt[DMX_LEN] == PKTTYPE_plain48) {
    if (valid_len)
      *valid_len = 48;
    return 48; // (48,48)
  }
  if (pkt[DMX_LEN] != PKTTYPE_chain20)
    return -1;
  int sz = 5;
  int content_len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);

  if (valid_len) {
    struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
    struct bipf_s *p = bipf_dict_getref(pending_ref, &i);
    if (p == NULL)
      *valid_len = content_len;
    else
      *valid_len = (48-20-sz) + 100 * p->u.list[0]->u.i;
  }
  return content_len;
  */
  /*
            if not seq in self.state['pend_sc']:
                return (content_len, content_len)
            available = (48-20-sz) + 100 * self.state['pend_sc'][seq][0]
            return (available, content_len)
  */
}

int ReplicaClass::get_chunk_cnt()
{
  _load_state();

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
  _load_state();

  return pending_ref;
}


struct bipf_s* ReplicaClass::get_next_in_chain(int seq)
{
  _load_state();

  struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
  return bipf_dict_getref(pending_ref, &i);
}


unsigned char* ReplicaClass::get_entry_pkt(int seq)
// results points to static buffer
{
  File f = _get_entry_start(seq);
  if (f == (File) NULL) {
    // Serial.printf("_get_entry_start %d failed\r\n", seq);
    return NULL;
  }
  static unsigned char buf[TINYSSB_PKT_LEN];
  int sz = f.read(buf, sizeof(buf));
  f.close();
  return sz == sizeof(buf) ? buf : NULL;
}


void ReplicaClass::hex_dump(int seq)
{
  _mk_fname(0); // log
  Serial.printf("# %s\r\n", fname);
  File f = MyFS.open(fname, FILE_READ);
  if (!f) {
    Serial.printf("** cannot open %s\r\n", fname);
    return;
  }
  unsigned char zer[16];
  memset(zer, 0, sizeof(zer));
  unsigned char buf[16];
  int was_zero_line = 0;

  for (int i = 0; true; i += sizeof(buf)) {
    int sz = f.read(buf, sizeof(buf));
    if (sz <= 0)
      break;
    if (sz == sizeof(buf) && !memcmp(buf, zer, sizeof(buf))) {
      was_zero_line++;
      if (was_zero_line == 2)
        Serial.printf("*\r\n");
      if (was_zero_line >= 2)
        continue;
    } else
      was_zero_line = 0;
    Serial.printf("%08x ", i);
    int j;
    for (j = 0; j < sz; j++) {
      Serial.printf(" %02x", buf[j]);
      if (j == 7)
        Serial.printf(" ");
    }
    if (j < sizeof(buf)) {
      if (j < 7) Serial.printf(" ");
      while (j++ < sizeof(buf))
        Serial.printf("   ");
    }
    Serial.printf("  |");
    for (j = 0; j < sz; j++)
      Serial.printf("%c", isprint(buf[j]) ? buf[j] : '.');
    Serial.printf("|\r\n");
  }
}

unsigned char* ReplicaClass::get_chunk_pkt(int seq, int chunk_nr)
{
  _load_state();

  if (seq < 1 || seq > max_seq_ref->u.i)
    return NULL;
  // check that we persisted this chunk, is not in a pending sidechain
  struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
  struct bipf_s *p = bipf_dict_getref(pending_ref, &i);
  if (p != NULL && chunk_nr >= p->u.list[0]->u.i)
    return NULL;
  // read the chunk
  static unsigned char buf[TINYSSB_PKT_LEN];
  File f = MyFS.open(fname, FILE_READ, false);
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


unsigned char* ReplicaClass::read(int seq, int *valid_len)
{
  File f = _get_entry_start(seq);
  if (f == (File) NULL)
    return NULL;
  // Serial.printf("#seq %d, pos=%d\r\n", seq, f.position());
  unsigned char pkt[TINYSSB_PKT_LEN];
  if (f.read(pkt, TINYSSB_PKT_LEN) != TINYSSB_PKT_LEN) {
    f.close();
    return NULL;
  }
  if (pkt[DMX_LEN] == PKTTYPE_plain48) {
    unsigned char *buf = (unsigned char*) malloc(48);
    memcpy(buf, pkt+DMX_LEN+1, 48);
    if (valid_len)
      *valid_len = 48;
    f.close();
    return buf;
  }
  if (pkt[DMX_LEN] != PKTTYPE_chain20) {
    f.close();
    return NULL;
  }
  int len;
  if (_get_content_len(pkt, seq, &len) < 0)
    return NULL;
  if (valid_len)
    *valid_len = len;

  int sz = 5;
  bipf_varint_decode(pkt, DMX_LEN+1, &sz);
  unsigned char *buf = (unsigned char*) malloc(len);
  if (len <= 28-sz) {
    memcpy(buf, pkt+8+sz, len);
    return buf;
  }
  memcpy(buf, pkt+8+sz, 28 - sz);
  len -= 28 - sz;
  unsigned char *cp = buf + 28 - sz;
  for (int blocks = (len + 99) / 100; blocks > 0; blocks--) {
    f.read(cp, len < 100 ? len : 100);
    cp += 100;
    len -= 100;
    f.read(pkt, 20); // we discard the hash value
  }
  f.close();
  return buf;

  /*
        with open(self.log_fname, 'rb') as f:
            pos = os.path.getsize(self.log_fname)

            cnt = self.state['max_seq'] - seq + 1
            while cnt > 0:
                f.seek(pos-4, os.SEEK_SET)
                pos = int.from_bytes(f.read(4), byteorder='big')
                cnt -= 1
            f.seek(pos, os.SEEK_SET)
            pkt = f.read(120)
            if pkt[7] == PKTTYPE_plain48:
                return pkt[8:56]
            if pkt[7] != PKTTYPE_chain20:
                return None
            chain_len, sz = bipf.varint_decode(pkt[8:])
            content = pkt[8+sz:36]
            blocks = (chain_len - len(content) + 99) // 100
            while blocks > 0:
                pkt = f.read(120)
                content += pkt[:100]
                blocks -= 1
        return content[:chain_len]
  */

}


// eof
