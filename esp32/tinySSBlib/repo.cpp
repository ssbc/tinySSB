// repo.cpp

// tinySSB for ESP32
// (c) 2022-2023 <christian.tschudin@unibas.ch>

#include "tinySSBlib.h"

#define FEED_RECS_FNAME "/feed_dir.bin"

// ----------------------------------------------------------------------

void Repo2Class::clean(char *path)
{
  for (int i = 0; i < rplca_cnt; i++) {
    Replica3Class *r = replicas[i];
    Serial.printf("erasing log %s\r\n", r->fname);
    MyFS.open(r->fname, "w").close();
    memcpy(feeds[i].mid, r->fid, HASH_LEN);
    memset(feeds[i].seq, 0, 8);
  }
  Serial.printf("storing new repo status in %s\r\n", FEED_RECS_FNAME);
  File f = MyFS.open(FEED_RECS_FNAME, "w");
  f.write((unsigned char*)feeds, rplca_cnt * sizeof(struct feedRecord_s));
  f.close();

  /*
  File fdir = MyFS.open(path);
  if (fdir) {
    File f = fdir.openNextFile(FILE_READ);
    while (f) {
      char *fn = strdup(f.path());
      f.close();
      Serial.printf("  removing %s\r\n", fn);
      MyFS.remove(fn);
      free(fn);
      f = fdir.openNextFile(FILE_READ);
    }
    fdir.close();
  }
  */
}

void Repo2Class::reset(char *path)
{
  // if (path == NULL) { path = FEED_DIR; }
  path = "/";
  Serial.printf("repo reset of path %s\r\n", path);
  this->clean(path);
  // if (path != NULL)
  //  MyFS.rmdir(path);
  MyFS.end();
  Serial.printf("formatting partition ..\r\n");
  MyFS.format();
  Serial.printf("restarting now\r\n");
  esp_restart(); // FIXME?? is this still necessary? if not, then we have to erase the in-memory GOset ...
}

void Repo2Class::load()
{
  // Serial.printf("length of feedRecord_s is %d\r\n", sizeof(struct feedRecord_s));
  int cnt = 0;
  if (!MyFS.exists(FEED_RECS_FNAME)) // create table of feeds
    MyFS.open(FEED_RECS_FNAME, "w").close();
  else {
    File f = MyFS.open(FEED_RECS_FNAME, "r");
    cnt = f.size() / sizeof(struct feedRecord_s);
    feeds = (struct feedRecord_s*) malloc(cnt * sizeof(struct feedRecord_s));
    f.read((unsigned char*)feeds, cnt * sizeof(struct feedRecord_s));
    f.close();
  }
  for (int i = 0; i < cnt; i++) {
    char fname[14];
    sprintf(fname, "/%08x.aol", feeds[i].fn);
    // Serial.printf("loading %s %s\r\n", to_hex(feeds[i].fid, 32), fname);

    uint16_t seq;
    uint32_t logsize = 0, pendscc = 0;
    memcpy((unsigned char*)&seq, feeds[i].seq, 2);
    memcpy(((unsigned char*)&logsize)+1, feeds[i].logsize, 3);
    memcpy(((unsigned char*)&pendscc)+1, feeds[i].pendscc, 3);
    seq = ntohs(seq);
    logsize = ntohl(logsize);
    pendscc = ntohl(pendscc);
    // Serial.printf("from FDIR: seq=%d, logsize=%d, pendscc=%d\r\n",
    //               seq, logsize, pendscc);
    struct Replica3Class *r = new Replica3Class(fname, feeds[i].fid,
                                                seq, feeds[i].mid,
                                                logsize, pendscc);
    r->fr_pos = i;
    replicas[rplca_cnt++] = r;
    theGOset->populate(feeds[i].fid);
    entry_cnt += seq;

    // arm DMXT
    unsigned char dmx_val[DMX_LEN];
    int ns = r->get_next_seq(dmx_val);
    theDmx->arm_dmx(dmx_val, incoming_entry, feeds[i].fid, /*ndx,*/ ns);
    // Serial.printf("   armed %s for %d.%d\r\n", to_hex(dmx_val, 7),
    //               ndx, f->next_seq);

    /*
    // arm CHKT
    struct bipf_s *p = r->get_open_chains();
    if (p != NULL && p->cnt > 0) {
      for (int j = 0; j < p->cnt; j++) {
        int seq = p->u.dict[2*j]->u.i;
        int cnr = p->u.dict[2*j+1]->u.list[0]->u.i;
        unsigned char *hptr = p->u.dict[2*j+1]->u.list[2]->u.buf;
        theDmx->arm_hsh(hptr, incoming_chunk, r->fid, seq, cnr,
                     cnr + p->u.dict[2*j+1]->u.list[1]->u.i);
      }
    }
    */
  }
  theGOset->populate(NULL); // triggers sorting, and setting the want_dmx
}


void Repo2Class::loop()
{
  /*
  static int done_chunk_counting = false;

  if (!done_chunk_counting) {
    int i;
    for (i = 0; i < rplca_cnt; i++)
      if (replicas[i]->chunk_cnt < 0) {
        int cnt = replicas[i]->load_chunk_cnt();
        chunk_cnt += cnt;
        // should re-sum all feed chunk counts
        theUI->refresh();
        break;
      }
    if (i >= rplca_cnt)
      done_chunk_counting = true;
  }
  */
}


void Repo2Class::create_replica(unsigned char *fid)
{
  // Serial.printf("create replica %s\r\n", to_hex(fid, 32));
  char fname[14];
  uint32_t fn;
  memcpy((unsigned char*)&fn, fid, sizeof(uint32_t));
  fn = ntohl(fn);
  while (1) {
    sprintf(fname, "/%08x.aol", fn);
    // Serial.printf("trying %s\r\n", fname);
    if (!MyFS.exists(fname))
      break;
    fn++;
  }
  // Serial.printf("   creating %s %s\r\n", to_hex(fid, 32), fname);
  // first we extend our directory
  rplca_cnt++;
  feeds = (struct feedRecord_s*) realloc((unsigned char*)feeds,
                                     rplca_cnt * sizeof(struct feedRecord_s));
  struct feedRecord_s *fr = feeds + rplca_cnt - 1;
  // memset((unsigned char*)fr, 0, sizeof(*fr));
  memcpy(fr->fid, fid, FID_LEN);
  memcpy(fr->mid, fid, HASH_LEN);
  memset(fr->seq, 0, 8); // 1x16bits + 2x32bits, only last 3 bytes in both cases
  fr->fn = fn;
  File f = MyFS.open(FEED_RECS_FNAME, "a");
  f.write((unsigned char*)fr, sizeof(*fr));
  // Serial.printf("appended feed_dir.bin size is now %d\r\n", f.size());
  f.close();

  // only after above storing, we create the append-only log
  MyFS.open(fname,"w").close();

  replicas[rplca_cnt-1] = new Replica3Class(fname, fr->fid, 0, fr->mid, 0, 0);
  replicas[rplca_cnt-1]->fr_pos = rplca_cnt-1;

  // arm DMXT
  unsigned char dmx_val[DMX_LEN];
  int ns = replicas[rplca_cnt-1]->get_next_seq(dmx_val);
  theDmx->arm_dmx(dmx_val, incoming_entry, replicas[rplca_cnt-1]->fid, ns);
  // Serial.printf("   armed %s for %d.%d\r\n", to_hex(dmx_val, 7),
  //               theGOset->_key_index(fid), ns);

  theUI->refresh();

  if (theGOset->goset_len > 0) {
    want_offs = esp_random() % theGOset->goset_len;
    chnk_offs = esp_random() % theGOset->goset_len;
  }
  
  want_is_valid = 0;
  chnk_is_valid = 0;
}


Replica3Class* Repo2Class::fid2replica(unsigned char* fid)
{
  int i;
  for (i = 0; i < rplca_cnt; i++)
    if (!memcmp(fid, replicas[i]->fid, FID_LEN))
      break;
  if (i >= rplca_cnt)
    return NULL;
  return replicas[i];
}


void Repo2Class::update_feed_rec(Replica3Class *r)
{
  // Serial.printf("update feed rec file, record is %d, fsize=%d\r\n", r->fr_pos, r->fsize);
  struct feedRecord_s *fr = feeds + r->fr_pos;
  memcpy(fr->mid, r->mid, HASH_LEN);
  unsigned short us = htons(r->seq);
  memcpy(fr->seq, (unsigned char*)&us, 2);
  uint32_t tmp = htonl(r->fsize);
  memcpy(fr->logsize, ((unsigned char*)&tmp)+1, 3);
  tmp = htonl(r->pendscc);
  memcpy(fr->pendscc, ((unsigned char*)&tmp)+1, 3);
  File f = MyFS.open(FEED_RECS_FNAME, "r+");
  if (!f)
    Serial.println("cannot open feed rec file");
  else {
    int rc = f.seek(r->fr_pos * sizeof(struct feedRecord_s), SeekSet);
    // Serial.printf("  seek %d returned %d\r\n", r->fr_pos * sizeof(struct feedRecord_s), rc);
    rc = f.write((unsigned char*)fr, sizeof(*fr));
    // Serial.printf("  write returned %d, size now %d\r\n", rc, f.size());
    f.close();
  }
}


/*
int Repo2Class::_fid2ndx(unsigned char* fid)
{
  int i;
  for (i = 0; i < rplca_cnt; i++)
    if (!memcmp(fid, feeds[i]->fid, FID_LEN))
      break;
  if (i >= rplca_cnt)
    return -1;
  return i;
}
*/

void Repo2Class::mk_want_vect()
{
  if (want_is_valid)
    return;

  String v = "";
  struct bipf_s *lptr = bipf_mkList();
  int encoding_len = bipf_encodingLength(lptr);

  bipf_list_append(lptr, bipf_mkInt(want_offs));

  int i;
  for (i = 0; i < theGOset->goset_len; i++) {
    unsigned int ndx = (want_offs + i) % theGOset->goset_len;
    unsigned char *fid = theGOset->get_key(ndx);
    Replica3Class *r = theRepo->fid2replica(fid);

    int ns = r->get_next_seq(NULL);
    struct bipf_s *bptr = bipf_mkInt(ns);
    encoding_len += bipf_encodingLength(bptr);
    bipf_list_append(lptr, bptr);

    v += (v.length() == 0 ? "[ " : " ") + String(ndx) + "." + String(ns);
    if (encoding_len > 100)
      break;
    io_loop();
    // io_dequeue();
  }
  want_offs = (want_offs + i + 1) % theGOset->goset_len;

  free(want_vect);
  if (lptr->cnt > 1) {
    want_len = bipf_encodingLength(lptr);
    want_vect = (unsigned char*) malloc(want_len);
    bipf_encode(want_vect, lptr);
    Serial.printf("   new WANT=%s ] %dB\r\n", v.c_str(), want_len);
  } else
    want_len = 0;

  bipf_free(lptr);
  want_is_valid = 1;
}


void Repo2Class::mk_chnk_vect()
{
  if (theGOset->goset_len == 0)
    chnk_len = 0;
  else {
    static int ndx = esp_random() % theGOset->goset_len;
    int old_ndx = ndx;
    int cnt = 0;
    String v = "";
    struct bipf_s *lptr = bipf_mkList();
    int encoding_len = bipf_encodingLength(lptr);

    do {
      struct chunk_needed_s table[4];
      ndx = (ndx+1) % theGOset->goset_len;
      Replica3Class *r = fid2replica(theGOset->get_key(ndx));
      io_loop();
      int old_pendscc = r->pendscc;
      int n = r->get_open_sidechains(4, table);
      if (old_pendscc != r->pendscc)
        update_feed_rec(r);
      for (int i = 0; i < n; i++) {
        // Serial.printf("  h=%s s=%d c=%d\r\n",
        //               to_hex(table[i].hash, HASH_LEN),
        //               table[i].snr, table[i].cnr);
        struct bipf_s *c = bipf_mkList();
        bipf_list_append(c, bipf_mkInt(ndx));
        bipf_list_append(c, bipf_mkInt(table[i].snr));
        bipf_list_append(c, bipf_mkInt(table[i].cnr));
        encoding_len += bipf_encodingLength(c);
        bipf_list_append(lptr, c);
        theDmx->arm_hsh(table[i].hash, incoming_chunk,
                        theGOset->get_key(ndx), table[i].snr, table[i].cnr, false);
        v += (v.length() == 0 ? "[ " : " ") + String(ndx) + "."
                           + String(table[i].snr) + '.' + String(table[i].cnr);
        if (encoding_len > 100)
          break;
      }
    } while (encoding_len <= 100 && old_ndx != ndx);
    free(chnk_vect);
    chnk_vect = NULL;
    if (lptr->cnt > 0) {
      chnk_len = bipf_encodingLength(lptr);
      chnk_vect = (unsigned char*) malloc(chnk_len);
      bipf_encode(chnk_vect, lptr);
      Serial.printf("   new C=%s ] %dB\r\n", v.c_str(), chnk_len);
    } else
      chnk_len = 0;
    bipf_free(lptr);
  }
}


// eof
