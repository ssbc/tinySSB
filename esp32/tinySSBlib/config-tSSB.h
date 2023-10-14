// config-tSSB.h

#if !defined(_INCLUDE_CONFIG_TSSB_H)
#define _INCLUDE_CONFIG_TSSB_H

#include "tinySSBlib.h"

struct name_value_s {
  char *field;
  int i_value;
  char *s_value;
};

#define CONFIG_FILENAME "/config.bipf"

extern char ssid[];

struct bipf_s* config_load(); // returns a BIPF dict with the persisted config dict
void           config_save(struct bipf_s *dict); // persist the BIPF dict
char*          config_apply(struct name_value_s *dict); // returns NULL or err

// ---------------------------------------------------------------------------

#if defined(HAS_LORA)

struct lora_config_s {
  char plan[8];        // name of frequency plan, ASCIIZ
  unsigned long  fr;   // in Hz
  unsigned int   bw;   // bandwidth, in Hz
  unsigned short sf;   // spreading factor (7-12)
  unsigned short cr;   // coding rate (5-8)
  unsigned short sw;   // sync word
  unsigned char  tx;   // tx power
};

extern struct lora_config_s lora_configs[];
extern short lora_configs_cnt;
extern struct lora_config_s *the_lora_config;

#endif // HAS_LORA

#endif // _INCLUDE_CONFIG_TSSB_H
