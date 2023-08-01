// util.h

// (c) 2022-2023 <christian.tschudin@unibas.ch>

#if !defined(_INCLUDE_UTIL_H)
#define _INCLUDE_UTIL_H

char* to_hex(unsigned char* buf, int len, int add_colon=0); // len must be <50
unsigned char* from_hex(char *hex, int len);

#endif

// eof
