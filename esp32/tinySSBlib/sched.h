// sched.h

#ifndef _INCLUDE_SCHED_H
#define _INCLUDE_SCHED_H

#include "tinySSBlib.h"


#define SCHED_PPS  1.5   // packets per second (more precise: slots per sec)
#define SCHED_SEC   60   // length of future
#define SCHED_MAX   10   // max number of scheduled packets

typedef void (*probe_fct_t)(unsigned char **pkt,
                            unsigned short *len,
                            unsigned short *millis);

class SchedClass {

public:
  SchedClass(probe_fct_t g, probe_fct_t p, probe_fct_t v, probe_fct_t c);

  void tick();
  void schedule_asap(unsigned char *pkt, unsigned short len,
                     unsigned char *dmx = NULL, struct face_s *face = NULL);

private:
  int _find_free_slot(int delay); // evicts pkt if delay > 0
  // int _find_free_pkts(); // check for duplicates
  probe_fct_t probe[4];
  short slots[(int)(SCHED_PPS * SCHED_SEC)];
  unsigned short curr_slot;
  unsigned char* packets[SCHED_MAX];
  unsigned short lengths[SCHED_MAX];
  struct face_s* faces[SCHED_MAX];
  unsigned long next_slot_millis;
};

#endif

// eof
