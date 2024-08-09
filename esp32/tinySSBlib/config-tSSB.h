// config-tSSB.h

#ifndef _INCLUDE_CONFIG_TSSB_H
#define _INCLUDE_CONFIG_TSSB_H

#include "tinySSBlib.h"


// #define CONFIG_FILENAME "/config.bipf"
#define CONFIG_FILENAME "/spiffs/config.ipf"

extern char ssid[];

struct bipf_s* config_load(); // returns a BIPF dict with the persisted config dict
void           config_save(struct bipf_s *dict); // persist the BIPF dict

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
