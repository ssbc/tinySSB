// replica.cpp

// tinySSB for ESP32
// (c) 2023 <christian.tschudin@unibas.ch>

// persistency for a single feed, using "2FPF" (two files per feed)

/*
  feeds are stored as one file, 'log.bin'

  a) the internal file structure is:

  [ [log_entry_block]* <4B-pendscc> <4B-maxseq> ]

  b) each log_entry_block has the structure:

   [ log_pkt chk_pkt chk_pkt ... <20B-MID> <4B-pktstart> ]
     ^                                            |
     |                                            |
     `--------------------------------------------'
         absolute position in file 

  c) in case of a side chain, the slot for the last chunk
     has its last 20 bytes used as follows:
     20 * 0x00  --> sidechain is complete, or
      8 * 0x?? <4B-have> <4B-left> <4B-next_pend_link>
               where <have> is the number of received chunks
               where <left> is the number of missing chunks

  d) The <pendscc> is the start of a linked list of
     log entries which have an incomplete side chain.
     - <pendscc> points to the first byte after the
       youngest log entry with incomplete side chain
     - inside the slot for the last chunk, the <next_pend_link>
       has the continuation of the linked list
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


static int _write_uint32(File f, uint32_t val)
{
  val = htonl(val);
  return f.write((unsigned char*)&val, sizeof(val));
}


static uint32_t _read_uint32(File f)
{
  uint32_t val;
  f.read((unsigned char*)&val, sizeof(val));
  return ntohl(val);
}


ReplicaClass::ReplicaClass(char *datapath, unsigned char *fID)
{
  memcpy(this->fid, fID, FID_LEN);

  fname = (char*) malloc(strlen(datapath) + 1 + 64 + 1 + 7 + 1); // "/log.bin"
  sprintf(fname, "%s/%s", FEED_DIR, to_hex(fid, FID_LEN, 0));
  if (!MyFS.exists(fname))
    MyFS.mkdir(fname);  // perhaps this is done automatically by LittleFS
  strcat(fname, "/log.bin");

  chunk_cnt = -1;

#ifdef ERASE
  // erase:
  _mk_fname(0); // log
  MyFS.open(fname, "w", true).close();
  _mk_fname(1); // frt
  MyFS.remove(fname);
#endif

  memcpy(mid, fid, HASH_LEN);
  if (!MyFS.exists(fname)) { // create log
    seq = 0;
    pendscc = 0;
    File f = MyFS.open(fname, "w", true);
    _write_uint32(f, 0);// pendscc
    _write_uint32(f, 0);// seq
    f.close();
  } else {
    File f = MyFS.open(fname, "r+", false);
    int sz = f.size();
    f.seek(sz - 2 * sizeof(int32_t), SeekSet);
    f.read((unsigned char*)&pendscc, sizeof(pendscc));
    pendscc = ntohl(pendscc);
    f.read((unsigned char*)&seq, sizeof(seq));
    seq = ntohl(seq);
    if (seq > 0) {
      f.seek(sz - 3 * sizeof(uint32_t) - HASH_LEN, SeekSet);
      f.read(mid, HASH_LEN);
    }
    f.close();
    Serial.printf("opened log %s, %dB, seq=%d\r\n", fname, sz, seq);
  }
}


int ReplicaClass::load_chunk_cnt() // summed over all sequence numbers
{
  File f = MyFS.open(fname, "r");
  if (!f)
    return 0;
  int cnt = 0, k = 0, s = seq;
  uint32_t endAddr = f.size() - 2 * sizeof(uint32_t);
  while (endAddr != 0) {
    if ((++k % 10) == 0)
      io_loop();
    f.seek(endAddr - sizeof(uint32_t));
    uint32_t startAddr = _read_uint32(f);
    if ((endAddr - startAddr) > (TINYSSB_PKT_LEN+HASH_LEN+sizeof(uint32_t))) {
      f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - HASH_LEN, SeekSet);
      unsigned char tmp[HASH_LEN];
      f.read(tmp, HASH_LEN);
      bool is_empty = true;
      for (int i = 0; i < HASH_LEN; i++)
        if (tmp[i] != 0) { is_empty = false; break; }
      int delta;
      if (is_empty)
        delta = (endAddr-startAddr - HASH_LEN - sizeof(uint32_t)) \
                                                       / TINYSSB_PKT_LEN - 1;
      else {
        f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - \
                                                3*sizeof(uint32_t), SeekSet);
        delta = _read_uint32(f);
      }
      // Serial.printf("delta for seq=%d is %d\r\n", s, delta);
      cnt += delta;
    }
    endAddr = startAddr;
    s--;
  }
  f.close();
  // Serial.printf("chunk count for fid=%d is %d\r\n",
  //               theGOset->_key_index(fid), cnt);
  chunk_cnt = cnt;
  return cnt;
}


int ReplicaClass::_get_content_len(unsigned char *pkt,int seq_nr,int *valid_len)
{
  if (pkt[DMX_LEN] == PKTTYPE_plain48) {
    if (valid_len)
      *valid_len = 48;
    return 48;
  }
  int sz = 5;
  int len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);
  if (valid_len) {
    *valid_len = (48-20-sz); // no chunks implemented so far
    /*    *valid_len = len;
    if (len > 48-20-sz) {
      // _load_state();
      struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
      struct bipf_s *p = bipf_dict_getref(pending_ref, &i);
      if (p != NULL)
        *valid_len = (48-20-sz) + 100 * p->u.list[0]->u.i;
    }
    */
  }
  return len;
}


File ReplicaClass::_open_at_start(int seq_nr, uint32_t *endAddr)
{
  // _load_state();
  if (seq_nr < 1 || seq_nr > seq)
    return (File) NULL;

  File f = MyFS.open(fname, "r+", false);
  if (!f)
    return (File) NULL;
  uint32_t pos = f.size() - 2*sizeof(uint32_t); // skip pendscc and seq
  int cnt = seq - seq_nr + 1;
  while (cnt-- > 0) { // we run this at least once
    if (endAddr)
      *endAddr = pos;
    // Serial.printf("pos=%d\r\n", pos);
    f.seek(pos-sizeof(uint32_t), SeekSet);
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


char ReplicaClass::ingest_entry_pkt(unsigned char *pkt) // True/False
{
  // Serial.println(String("incoming entry for log ") + to_hex(fid, FID_LEN, 0))  // check dmx
  // _load_state();  
  unsigned char dmx_val[DMX_LEN];
  compute_dmx(dmx_val, fid, seq + 1, mid);
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
  memcpy(mid, h256, HASH_LEN); // =msgID
  // t2 = millis(); durations[2] = t2 - t1; t1 = t2;
  io_loop();

  File f = MyFS.open(fname, "r+"); // modify and append
  // Serial.printf("   appending %d.%d at %d\r\n", theGOset->_key_index(fid),
  //                max_seq_ref->u.i + 1, f.position());
  f.seek(f.size() - 2*sizeof(uint32_t), SeekSet); // skip pendscc and seq
  uint32_t pkt_start = f.position();
  f.write(pkt, TINYSSB_PKT_LEN);
  uint32_t ch_cnt = 0, tmp;
  if (pkt[DMX_LEN] == PKTTYPE_chain20) {
    int sz = 5;
    int content_len = bipf_varint_decode(pkt, DMX_LEN+1, &sz);
    content_len -= 48 - HASH_LEN - sz;
    // ptr = pkt[36:56] --> hash of first chunk
    ch_cnt = (content_len + 99) / 100;
    // Serial.printf("  content_len=%d, cc=%d\r\n", content_len, ch_cnt);
  }
  if (ch_cnt > 0) {
    unsigned char empty[TINYSSB_PKT_LEN];
    memset(empty, 0, sizeof(empty));
    for (int i = 1; i < ch_cnt; i++)
      f.write(empty, sizeof(empty));
    f.write(empty, TINYSSB_PKT_LEN - HASH_LEN - 3 * sizeof(uint32_t)); // fill
    f.write(pkt+36, HASH_LEN);   // hash of first side chain chunk
    _write_uint32(f, 0);         // zero chunks obtained so far
    _write_uint32(f, ch_cnt); // number of chunks left:
    _write_uint32(f, pendscc);   // old pendscc position:
    // -- this ends the last chunk
  }
  f.write(mid, HASH_LEN);
  _write_uint32(f, pkt_start);
  // -- this ends the log entry block
  if (ch_cnt != 0) // new pendscc
    pendscc = f.position();
  _write_uint32(f, pendscc);
  _write_uint32(f, ++seq);
  f.close();

  io_loop();
  // Serial.printf("end of ingest_entry_pkt\r\n");
  return 1;
}


char ReplicaClass::ingest_chunk_pkt(unsigned char *pkt, int snr, int *cnr) // True/False
{
  /* checking hash match should not be necessary, we filtered at CHKT level
  unsigned char h[crypto_hash_sha256_BYTES];
  crypto_hash_sha256(h, pkt, TINYSSB_PKT_LEN);
  */
  uint32_t endAddr;
  File f = _open_at_start(snr, &endAddr);
  if (!f)
    return 0;
  uint32_t startAddr = f.position();
  if ((endAddr - startAddr) <= (TINYSSB_PKT_LEN+HASH_LEN+sizeof(uint32_t))) {
    // that's bad: packet has no sidechain
    f.close();
    return 0;
  }
  // test final hash-of-next-chunk field
  f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - HASH_LEN, SeekSet);
  unsigned char tmp[HASH_LEN];
  f.read(tmp, HASH_LEN);
  bool is_empty = true;
  for (int i = 0; i < HASH_LEN; i++)
    if (tmp[i] != 0) { is_empty = false; break; }
  if (is_empty) {
    // that's bad: sidechain already complete
    f.close();
    return 0;
  }
  // read side chain status
  f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - 3*sizeof(uint32_t), SeekSet);
  uint32_t have = _read_uint32(f);
  uint32_t left = _read_uint32(f);
  uint32_t pscc = _read_uint32(f);
  f.seek(startAddr + (1+have) * TINYSSB_PKT_LEN, SeekSet);
  f.write(pkt, TINYSSB_PKT_LEN);
  if (left == 1) { // this was the last chunk, trim the pending chain
    Serial.printf("  chain %d.%d.%d complete\r\n",
                  theGOset->_key_index(fid), snr, *cnr);
    if (pendscc == endAddr) {
      pendscc = pscc;
      f.seek(f.size() - 2*sizeof(uint32_t));
      _write_uint32(f, pendscc);
    } else if (pendscc != 0) {
      uint32_t pos = pendscc;
      // pos has start if linked list of open side chains
      // (i.e., pos points to the end of the respective log entry)
      while (pos != 0) {
        if (!f.seek(pos - HASH_LEN - 2*sizeof(uint32_t), SeekSet))
          break;
        uint32_t tmp = _read_uint32(f); // continuation position
        if (tmp == endAddr) { // found it, replace it
          if (!f.seek(pos - HASH_LEN - 2*sizeof(uint32_t), SeekSet))
            break;
          _write_uint32(f, pscc);
          break;
        }
        pos = tmp;
      }
    }
  } else { // update side chain status
    Serial.printf("  writing new chain status %d.%d.%d: have=%d left=%d\r\n",
                  theGOset->_key_index(fid), snr, *cnr, have+1, left-1);
    f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - 3*sizeof(uint32_t), SeekSet);
    _write_uint32(f, have+1);
    _write_uint32(f, left-1);
  }
  f.close();
  chunk_cnt++;
  return 1;
}


int ReplicaClass::get_next_seq(unsigned char *dmx) // returns seq and DMX
{
  if (dmx)
    compute_dmx(dmx, fid, seq + 1, mid);
  return seq + 1;
}


int ReplicaClass::get_content_len(int seq_nr, int *valid_len)
{
  unsigned char *pkt = this->get_entry_pkt(seq_nr);
  if (!pkt)
    return -1;
  if (pkt[DMX_LEN] != PKTTYPE_plain48 && pkt[DMX_LEN] != PKTTYPE_chain20)
    return -1;
  return _get_content_len(pkt, seq_nr, valid_len);
}


/*
struct bipf_s* ReplicaClass::get_open_chains() // return a dict
{
  // _load_state();

  return pending_ref;
}
*/


/*
struct bipf_s* ReplicaClass::get_next_in_chain(int seq)
{
  // _load_state();

  struct bipf_s i = { BIPF_INT, {}, {.i = seq} };
  return bipf_dict_getref(pending_ref, &i);
}
*/


unsigned char* ReplicaClass::get_entry_pkt(int seq_nr)
// results points to static buffer
{
  File f = _open_at_start(seq_nr);
  if (f == (File) NULL) {
    // Serial.printf("_open_at_start %d failed\r\n", seq_nr);
    return NULL;
  }
  static unsigned char buf[TINYSSB_PKT_LEN];
  int sz = f.read(buf, sizeof(buf));
  f.close();
  return sz == sizeof(buf) ? buf : NULL;
}


unsigned char* ReplicaClass::get_chunk_pkt(int seq_nr, int chunk_nr)
{
  if (seq_nr < 1 || seq_nr > seq)
    return NULL;
  uint32_t endAddr;
  File f = _open_at_start(seq_nr, &endAddr);
  if (!f)
    return NULL;
  uint32_t startAddr = f.position();
  if ((endAddr - startAddr) <= (TINYSSB_PKT_LEN+HASH_LEN+sizeof(uint32_t))) {
    f.close();
    return NULL;
  }
  f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - HASH_LEN, SeekSet);
  unsigned char tmp[HASH_LEN];
  f.read(tmp, HASH_LEN);
  bool is_empty = true;
  unsigned int have;
  for (int i = 0; i < HASH_LEN; i++)
    if (tmp[i] != 0) { is_empty = false; break; }
  if (is_empty)
    have = (endAddr-startAddr - HASH_LEN - sizeof(uint32_t))            \
                                                       / TINYSSB_PKT_LEN - 1;
  else {
    f.seek(endAddr - sizeof(uint32_t) - HASH_LEN -                      \
                                                3*sizeof(uint32_t), SeekSet);
    have = _read_uint32(f);
  }
  if (chunk_nr >= have) {
    f.close();
    return NULL;
  }
  static unsigned char buf[TINYSSB_PKT_LEN];
  f.seek(startAddr + TINYSSB_PKT_LEN * (1 + chunk_nr), SeekSet);
  int sz = f.read(buf, sizeof(buf));
  f.close();
  return sz == sizeof(buf) ? buf : NULL;
}


int ReplicaClass::get_open_sidechains(int max_cnt, struct chunk_needed_s *pcn)
{
  if (seq == 0)
    return 0;
  File f = MyFS.open(fname, "r");
  f.seek(f.size() - 2*sizeof(uint32_t));
  f.close();
  uint32_t pos = _read_uint32(f);
  if (pos == 0)
    return 0;
  int cnt = 0;
  uint32_t old_iter;
  if (pssc_iter == 0) {
    old_iter = 1;
    pssc_iter = seq;
  } else {
    old_iter = pssc_iter;
    if (--pssc_iter == 0)
      pssc_iter = seq;
  }
  uint32_t endAddr;
  f = _open_at_start(pssc_iter, &endAddr);
  uint32_t startAddr = f.position();
  do {
    if ((endAddr - startAddr) > (TINYSSB_PKT_LEN+HASH_LEN+sizeof(uint32_t))) {
      f.seek(endAddr - sizeof(uint32_t) - HASH_LEN - HASH_LEN, SeekSet);
      unsigned char tmp[HASH_LEN];
      f.read(tmp, HASH_LEN);
      bool is_empty = true;
      for (int i = 0; i < HASH_LEN; i++)
        if (tmp[i] != 0) { is_empty = false; break; }
      if (!is_empty) { // side chain not completed yet
        pcn[cnt].snr = pssc_iter;
        f.seek(endAddr - sizeof(uint32_t) - HASH_LEN -                  \
                                                3*sizeof(uint32_t), SeekSet);
        pcn[cnt].cnr = _read_uint32(f); // have
        if (pcn[cnt].cnr == 0)
          f.seek(startAddr + 36);
        else
          f.seek(startAddr + TINYSSB_PKT_LEN * (1 + pcn[cnt].cnr) - HASH_LEN);
        f.read(pcn[cnt].hash, HASH_LEN);
        cnt++;
      }
      // else Serial.printf("    s=%d has full sidechain\r\n", pssc_iter);
    }
    // else Serial.printf("    s=%d has no sidechain\r\n", pssc_iter);
    if (startAddr != 0) {
      endAddr = startAddr;
      f.seek(endAddr - sizeof(uint32_t));
      startAddr = _read_uint32(f);
      pssc_iter--;
    } else {
      f.close();
      pssc_iter = seq;
      f = _open_at_start(pssc_iter, &endAddr);
      startAddr = f.position();
    }
  } while (cnt < max_cnt && pssc_iter != old_iter);
  f.close();
  return cnt;
}


void ReplicaClass::hex_dump(int seq)
{
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


int ReplicaClass::get_mid(int seq_nr, unsigned char *mid_ptr)
{
  if (seq_nr < 1 || seq_nr > seq || mid_ptr == NULL)
    return 0;
  if (seq_nr == seq) {
    memcpy(mid_ptr, mid, HASH_LEN);
    return 1;
  }
  uint32_t endAddr;
  File f = _open_at_start(seq_nr, &endAddr);
  f.seek(endAddr - sizeof(uint32_t) - HASH_LEN);
  f.read(mid_ptr, HASH_LEN);
  f.close();
  return 1;
}

unsigned char* ReplicaClass::read(int seq_nr, int *valid_len)
{
  File f = _open_at_start(seq_nr);
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
  return NULL;
  /*
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
  */
}


// eof
