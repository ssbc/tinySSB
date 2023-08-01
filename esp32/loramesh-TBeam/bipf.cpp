// bipf.cpp

// tinySSB for ESP32
// 2022-2023 <christian.tschudin@unibas.ch>

#include <Arduino.h>
#include <string.h>

#include "bipf.h"
#include "util.h"

#define TAG_SIZE 3
#define TAG_MASK 7

// -------------------------------------------------------------------------

int bipf_equal(struct bipf_s *a, struct bipf_s *b)
{
  if (a == NULL || b == NULL || a->typ != b->typ)
    return 0;
  switch(a->typ) {
    case BIPF_STRING:
    case BIPF_BYTES:
      return a->cnt == b->cnt && !memcmp(a->u.buf, b->u.buf, a->cnt);
    case BIPF_INT:
    case BIPF_BOOLNONE:
      return a->u.i == b->u.i;
    case BIPF_DOUBLE:
      return a->u.d == b->u.d;
    case BIPF_LIST:
      if (a->cnt != b->cnt)
        return 0;
      for (int i = 0; i < a->cnt; i++)
        if (!bipf_equal(a->u.list[i], b->u.list[i]))
          return 0;
      return 1;
    case BIPF_DICT:
      if (a->cnt != b->cnt)
        return 0;
      for (int i = 0; i < a->cnt; i++)
        if (!bipf_equal(a->u.dict[2*i], b->u.dict[2*i]) ||
            !bipf_equal(a->u.dict[2*i + 1], b->u.dict[2*i + 1]))
          return 0;
      return 1;
    default:
      break;
  }
  return 0;
}

struct bipf_s* bipf_mkBool(int flag)
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_BOOLNONE;
  bptr->u.i = flag ? 1 : 0;
  return bptr;  
}

struct bipf_s* bipf_mkBytes(unsigned char *buf, int len)
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_BYTES;
  bptr->cnt = len;
  bptr->u.buf = (unsigned char*) malloc(len);
  memcpy(bptr->u.buf, buf, len);
  return bptr;
}

struct bipf_s* bipf_mkInt(int val)
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_INT;
  bptr->u.i = val;
  return bptr;
}

struct bipf_s* bipf_mkNone()
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_BOOLNONE;
  bptr->u.i = -1;
  return bptr;  
}

struct bipf_s* bipf_mkList()
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_LIST;
  return bptr;
}

struct bipf_s* bipf_mkDict()
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_DICT;
  return bptr;
}

struct bipf_s* bipf_mkString(char *cp)
{
  struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
  bptr->typ = BIPF_STRING;
  bptr->cnt = strlen(cp);
  bptr->u.str = (char*) malloc(bptr->cnt);
  memcpy(bptr->u.str, cp, bptr->cnt);
  return bptr;
}

void bipf_list_append(struct bipf_s *lptr, struct bipf_s *e)
{
  lptr->cnt++;
  // Serial.printf("realloc list\n\r");
  lptr->u.list = (struct bipf_s**) realloc(lptr->u.list, lptr->cnt * sizeof(struct bipf_s*));
  lptr->u.list[lptr->cnt - 1] = e;
}

struct bipf_s* bipf_list_getref(struct bipf_s *lptr, int i)
{
  return lptr->u.list[i];
}

void bipf_dict_set(struct bipf_s *dptr, struct bipf_s *k, struct bipf_s *v)
{
  int i;
  for (i = 0; i < dptr->cnt; i++)
    if (bipf_equal(k, dptr->u.dict[2*i]))
        break;
  if (i < dptr->cnt) {
    bipf_free(dptr->u.dict[2*i]);
    bipf_free(dptr->u.dict[2*i + 1]);
  } else {
    dptr->cnt++;
    // Serial.printf("realloc dict new cnt=%d\n\r", dptr->cnt);
    dptr->u.dict = (struct bipf_s**) realloc(dptr->u.dict, dptr->cnt * 2 * sizeof(struct bipf_s*));
  }
  // Serial.printf("dict set k=%p v=%p\n\r", k, v);
  dptr->u.dict[2*i] = k;
  dptr->u.dict[2*i + 1] = v;
}

struct bipf_s* bipf_dict_getref(struct bipf_s *dptr, struct bipf_s *k)
{
  for (int i = 0; i < dptr->cnt; i++)
    if (bipf_equal(k, dptr->u.dict[2*i])) {
      // Serial.printf("dict_getref %d %s=", i, to_hex((unsigned char*) (k->u.str), k->cnt));
      // Serial.printf("%s\r\n", to_hex((unsigned char*) (dptr->u.dict[2*i]->u.str), dptr->u.dict[2*i]->cnt));
      return dptr->u.dict[2*i + 1];
    }
  return NULL;
}

void bipf_dict_delete(struct bipf_s *dptr, struct bipf_s *k)
{
  // Serial.printf("BIPF dict delete %d\r\n", dptr->cnt);
  for (int i = 0; i < dptr->cnt; i++)
    if (bipf_equal(k, dptr->u.dict[2*i])) {
      bipf_free(dptr->u.dict[2*i]);
      bipf_free(dptr->u.dict[2*i + 1]);
      // dptr->u.dict[2*i] = NULL;
      // dptr->u.dict[2*i + 1] = NULL;
      if (i != dptr->cnt-1)
        memmove(dptr->u.dict + 2*i, dptr->u.dict + 2*i + 2,
                2*(dptr->cnt - i - 1) * sizeof(struct bipf_s*));
      dptr->cnt--;
      return;
    }
}

void bipf_free(struct bipf_s *bptr)
{
  struct bipf_s **pp;

  switch (bptr->typ) {
    case BIPF_BYTES:
    case BIPF_STRING:
      free(bptr->u.buf);
      break;
    case BIPF_INT:
      break;
    case BIPF_LIST:
      pp = bptr->u.list;
      while (bptr->cnt-- > 0)
        bipf_free(*pp++);
      free(bptr->u.list);
      break;
    case BIPF_DICT:
      pp = bptr->u.dict;
      while (bptr->cnt-- > 0) {
        bipf_free(*pp++);
        bipf_free(*pp++);
      }
      free(bptr->u.dict);
      break;
    default:
      // Serial.println("can't free BIPF for typ=" + String(bptr->typ));
      break;
  }
  free(bptr);
}

// ------------------------------------------------------------------------------

unsigned int bipf_varint_decode(unsigned char *buf, int pos, int *lptr)
{
  unsigned int val = 0;
  int shift = 0;
  int old = pos;
  int end = pos + *lptr;
  while (pos < end) {
    unsigned int b = buf[pos];
    val |= (b & 0x7f) << shift;
    if ((b & 0x80) == 0)
      break;
    shift += 7;
    pos += 1;
  }
  *lptr = pos - old + 1;
  return val;
}

unsigned int bipf_varint_encoding_length(unsigned int val)
{
  int cnt = 1;
  while (1) {
    val >>= 7;
    if (val == 0)
      return cnt;
    cnt++;
  }
}

unsigned int bipf_varint_encode(unsigned char *buf, unsigned int val)
{
  unsigned char *old = buf - 1;
  while (1) {
    *buf = val & 0x7f;
    val >>= 7;
    if (val != 0)
      *buf |= 0x80;
    else
      return buf - old;
    buf++;
  }
}

// ------------------------------------------------------------------------------

struct bipf_s* _bipf_dec_inner(int tag, unsigned char *buf, int pos, int *lptr)
{
  // Serial.println(String(" inner ") + to_hex(buf+pos, 6) + " pos=" + String(pos) + " lim=" + String(*lptr));
  if (tag == BIPF_BOOLNONE) { //  i.e., length is 0
    struct bipf_s *bptr = (struct bipf_s*) malloc(sizeof(struct bipf_s));
    bptr->typ = BIPF_BOOLNONE;
    *lptr = 0;
    return bptr;
  }
  int t = tag & TAG_MASK;
  int sz = tag >> TAG_SIZE;
  int lim = pos + sz;
  if (lim > *lptr)
    return NULL;
  // Serial.println(" inner dec: t=" + String(t) + " sz=" + String(sz) + " pos=" + String(pos));
  switch(t) {
    case BIPF_BOOLNONE: {
      struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
      bptr->typ = BIPF_BOOLNONE;
      bptr->u.i = sz == 0 ? -1 : (buf[pos] ? 1 : 0);
      *lptr = sz;
      return bptr;
    }
    case BIPF_BYTES: {
      struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
      bptr->typ = BIPF_BYTES;
      bptr->cnt = sz;
      bptr->u.buf = (unsigned char*) malloc(sz);
      memcpy(bptr->u.buf, buf + pos, sz);
      *lptr = sz;
      return bptr;
    }
    case BIPF_INT: {
      // Serial.println(" int");
      int old = pos;
      int val = 0;
      int i;
      for (i = 0; i < 8*sz; i += 8) // little endian
        val |= buf[pos++] << i;
      int m = 1 << i+7;
      struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
      bptr->typ = BIPF_INT;
      bptr->u.i = (val & m) == 0 ? val : val - (m << 1);
      // Serial.println("bipf INT " + String(bptr->u.i));
      *lptr = pos - old;
      return bptr;
    }
    case BIPF_LIST: {
      int old = pos; 
      struct bipf_s *bptr = bipf_mkList();
      while (pos < lim) {
        // Serial.println("list loop pos=" + String(pos));
        int szL = lim;
        unsigned int tagL = bipf_varint_decode(buf, pos, &szL);
        // Serial.println("bipf tagL " + String(tag) + "/" + String(tag>>3));
        pos += szL;
        szL = lim;
        struct bipf_s *e = _bipf_dec_inner(tagL, buf, pos, &szL);
        if (e == NULL)
          return NULL;
        bipf_list_append(bptr, e);
        pos += szL;
      }
      *lptr = pos - old;
      // Serial.println("cnt=" + String(bptr->cnt));
      return bptr;
    }
    case BIPF_DICT: {
      int old = pos; 
      struct bipf_s *bptr = bipf_mkDict();
      while (pos < lim) {
        // Serial.println("dict loop pos=" + String(pos));
        int szK = lim;
        unsigned int tagK = bipf_varint_decode(buf, pos, &szK);
        pos += szK;
        szK = lim;
        struct bipf_s *k = _bipf_dec_inner(tagK, buf, pos, &szK);
        if (k == NULL)
          return NULL;
        pos += szK;
        int szV = lim;
        unsigned int tagV = bipf_varint_decode(buf, pos, &szV);
        pos += szV;
        szV = lim;
        struct bipf_s *v = _bipf_dec_inner(tagV, buf, pos, &szV);
        if (v == NULL)
          return NULL;
        bipf_dict_set(bptr, k, v);
        pos += szV;
      }
      *lptr = pos - old;
      // Serial.println("cnt=" + String(bptr->cnt));
      return bptr;
    }
    case BIPF_STRING: {
      struct bipf_s *bptr = (struct bipf_s*) calloc(1, sizeof(struct bipf_s));
      bptr->typ = BIPF_STRING;
      bptr->cnt = sz;
      bptr->u.buf = (unsigned char*) malloc(sz+1);
      memcpy(bptr->u.str, buf + pos, sz);
      bptr->u.str[sz] = '\0';
      *lptr = sz;
      return bptr;
    }
    default:
      Serial.printf("bipf not implemented or wrong tag=%d, pos=%d\r\n", tag, pos);
      break;
  }
  return NULL; // _type2decoder[t](buf, pos, tag >> _TAG_SIZE, lptr);
}

// ------------------------------------------------------------------------------

struct bipf_s* bipf_decode(unsigned char *buf, int pos, int *lptr)
{
  // Serial.printf("decode %s\r\n", to_hex(buf+pos, *lptr, 0));
  // read the next value from buffer at start.
  // returns a tuple with the value and the consumed bytes
  int sz1 = *lptr;
  int tag = bipf_varint_decode(buf, pos, &sz1);
  int sz2 = *lptr;
  // Serial.printf("bipf tag %d/%d sz1=%d sz2=%d\r\n", tag&7, tag>>3, sz1, sz2);
  struct bipf_s *val = _bipf_dec_inner(tag, buf, pos + sz1, &sz2);
  *lptr = sz1 + sz2;
  return val;
}

int bipf_bodyLength(bipf_s *bptr)
{
  // returns the length needed for the body of the encode value
  // if val == None:
  //    return 1
  switch (bptr->typ) { // length of body
    case BIPF_BOOLNONE:
      return bptr->u.i >= 0 ? 1 : 0;
    case BIPF_BYTES:
    case BIPF_STRING:
      return bptr->cnt;
    case BIPF_INT: {
      int val = bptr->u.i;
      if (val < 0) val = -val - 1;
      int sz = 1;
      val >>= 8;
      while (val > 0) {
        sz++;
        val >>= 8;
      }
      // Serial.println("int " + String(bptr->u.i) + " size=" + String(sz));
      return sz;
    }
    case BIPF_LIST: {
      int sz = 0;
      for (int i = 0; i < bptr->cnt; i++)
        sz += bipf_encodingLength(bptr->u.list[i]);
      // Serial.println("list " + String(sz));
      return sz;
    }
    case BIPF_DICT: {
      int sz = 0;
      for (int i = 0; i < bptr->cnt; i++) {
        sz += bipf_encodingLength(bptr->u.dict[2*i]);
        sz += bipf_encodingLength(bptr->u.dict[2*i+1]);
      }
      // Serial.println("dict " + String(sz));
      return sz;
    }
    default:
    break;
  }
  // Serial.println("unknown BIPF type " + String(bptr->typ));
  return -1;
}

int bipf_encodingLength(bipf_s *bptr)
{
  // returns the length needed to encode the value (header and body)
  int sz = bipf_bodyLength(bptr);
  int len = bipf_varint_encoding_length(sz << TAG_SIZE) + sz;
  // Serial.println(String(bptr->typ) + " sz=" + String(sz) + ", encLen=" + String(len));
  return len;
}

void bipf_encodeBody(unsigned char *buf, bipf_s *bptr)
{
  switch (bptr->typ) { // length of body
    case BIPF_BOOLNONE:
      *buf = bptr->u.i >= 0 ? 1 : 0;
      return;
    case BIPF_BYTES:
    case BIPF_STRING:
      memcpy(buf, bptr->u.buf, bptr->cnt);
      return;
    case BIPF_INT: {
      int val = bptr->u.i;
      if (val < 0) val = -val - 1;
      *buf++ = (unsigned int) val & 0xff;
      val >>= 8;
      while (val > 0) {
        *buf++ = (unsigned int) val & 0xff;
        val >>= 8;
      }
      return;
    }
    case BIPF_LIST:
      for (int i = 0; i < bptr->cnt; i++)
        buf += bipf_encode(buf, bptr->u.list[i]);
      return;
    case BIPF_DICT:
      for (int i = 0; i < bptr->cnt; i++) {
        buf += bipf_encode(buf, bptr->u.dict[2*i]);
        buf += bipf_encode(buf, bptr->u.dict[2*i+1]);
      }
      return;
    default:
      break;
  }
  // Serial.println("encodeBody: unknown BIPF type " + String(bptr->typ));
}

int bipf_encode(unsigned char *buf, struct bipf_s *bptr)
{
  int sz = bipf_bodyLength(bptr);
  int tagInt = sz << TAG_SIZE | bptr->typ;
  int tagLen = bipf_varint_encode(buf, tagInt);
  if (sz > 0)
    bipf_encodeBody(buf+tagLen, bptr);
  return tagLen + sz;
}

unsigned char* bipf_dumps(struct bipf_s *bptr, int *len)
{
  *len = bipf_encodingLength(bptr);
  unsigned char *buf = (unsigned char*) malloc(*len);
  bipf_encode(buf, bptr);
  return buf;
}

struct bipf_s* bipf_loads(unsigned char *buf, int len)
{
  return bipf_decode(buf, 0, &len);
}

struct bipf_s* str2bipf(char *s) // returns addr of a static (!) BIPF obj
{
  static bipf_s e = { BIPF_STRING };
  e.cnt = strlen(s);
  e.u.str = s;
  return &e;
}

String bipf2String(struct bipf_s *bptr)
{
  String v;
  struct bipf_s **pp;

  switch (bptr->typ) {
    case BIPF_BYTES:
      return "0x...";
    case BIPF_STRING:
      return "\"" + String(bptr->u.str) + "\"";
    case BIPF_BOOLNONE:
      if  (bptr->u.i < 0)
        return "none";
      if  (bptr->u.i == 0)
        return "false";
      return "true";
    case BIPF_DOUBLE:
      return String(bptr->u.d);
    case BIPF_INT:
      return String(bptr->u.i);
    case BIPF_LIST:
      v = "[";
      pp = bptr->u.list;
      for (int i = 0; i < bptr->cnt; i++) {
        if (v.length() != 1)
          v += ",";
        v += bipf2String(*pp++);
      }
      return v + "]";
    case BIPF_DICT:
      v = "{";
      pp = bptr->u.dict;
      for (int i = 0; i < bptr->cnt; i++) {
        if (v.length() != 1)
          v += ",";
        v += bipf2String(*pp++) + ":";
        v += bipf2String(*pp++);
      }
      return v + "}";
    default:
      // Serial.println("can't render typ=" + String(bptr->typ));
      break;
  }
  return "?";
}

// eof
