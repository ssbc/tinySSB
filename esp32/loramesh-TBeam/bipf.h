// bipf.h

// tinySSB for ESP32
// 2022-2023 <christian.tschudin@unibas.ch>


#if !defined(_INCLUDE_BIPF_H)
#define _INCLUDE_BIPF_H

struct bipf_s {
  unsigned char typ;
  unsigned short cnt; // for list, dict, string and bytes
  union {
    int i;
    double d;
    unsigned char *buf; // cnt has length of (allocated) byte array
    char *str;          // cnt has number of (allocated) characters
    struct bipf_s **list;
    struct bipf_s **dict;
  } u;
};

enum BIPF_TYPES {
  BIPF_STRING,
  BIPF_BYTES,
  BIPF_INT,
  BIPF_DOUBLE,
  BIPF_LIST,
  BIPF_DICT,
  BIPF_BOOLNONE,
  BIPF_RESERVED,
  BIPF_EMPTY = 255
};

// -------------------------------------------------------------------------

int bipf_equal(struct bipf_s *a, struct bipf_s *b);
struct bipf_s* bipf_mkBool(int flag);
struct bipf_s* bipf_mkBytes(unsigned char *buf, int len);
struct bipf_s* bipf_mkInt(int val);
struct bipf_s* bipf_mkNone();
struct bipf_s* bipf_mkList();
struct bipf_s* bipf_mkDict();
struct bipf_s* bipf_mkString(char *cp);
void bipf_list_append(struct bipf_s *lptr, struct bipf_s *e);
struct bipf_s* bipf_list_getref(struct bipf_s *lptr, int i);
struct bipf_s* bipf_dict_getref(struct bipf_s *dptr, struct bipf_s *k);
void bipf_dict_set(struct bipf_s *dptr, struct bipf_s *k, struct bipf_s *v);
void bipf_dict_delete(struct bipf_s *dptr, struct bipf_s *k);
void bipf_free(struct bipf_s *bptr);

unsigned int bipf_varint_decode(unsigned char *buf, int pos, int *lptr);
unsigned int bipf_varint_encoding_length(unsigned int val);
unsigned int bipf_varint_encode(unsigned char *buf, unsigned int val);

int bipf_encodingLength(bipf_s *bptr);
int bipf_encode(unsigned char *buf, struct bipf_s *bptr);
unsigned char* bipf_dumps(struct bipf_s *bptr, int *len);
struct bipf_s* bipf_loads(unsigned char *buf, int len);

struct bipf_s* str2bipf(char *s); // return static bipf_s for s
struct String bipf2String(struct bipf_s *bptr);

#endif // _INCLUDE_BIPF_H

// eof
