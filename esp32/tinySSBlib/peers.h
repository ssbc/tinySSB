// peers.h

// sends "I am here beacons", permits to create a list of visible peers

#ifndef _INCLUDE_PEERS_H
#define _INCLUDE_PEERS_H

#include "config.h"


class PeersClass {
  
public:
  PeersClass(/* filename for log */);
  void probe_for_peers_beacon(unsigned char **pkt,
                              unsigned short *len,
                              unsigned short *reprobe_in_millis);
  void incoming_req(unsigned char *pkt, int len, unsigned char *aux,
                    struct face_s *face);
  void incoming_rep(unsigned char *pkt, int len, unsigned char *aux,
                    struct face_s *face);
  // void save2log(char *s)

  unsigned char peer_dmx_req[7];
  unsigned char peer_dmx_rep[7];
private:
  int peer_clock;

  unsigned long peer_next;
  unsigned long peer_refresh;

  // char *fname;
  // File peer_log;

};

#endif
