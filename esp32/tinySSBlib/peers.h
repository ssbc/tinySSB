// peers.h

// sends "I am here beacons", permits to create a list of visible peers

#ifndef _INCLUDE_PEERS_H
#define _INCLUDE_PEERS_H

#include "tinySSBlib.h"

struct peer_s {
  char id[5];
  long when;
  int rssi;
  float snr;
};
#define MAX_HEARD_PEERS 8


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
  double get_peer_density(); // avg number of peers heard since less than 60 sec
  // void save2log(char *s)

  unsigned char peer_dmx_req[7];
  unsigned char peer_dmx_rep[7];

  struct peer_s heard_peers[MAX_HEARD_PEERS];

private:
  void _update_peer(char *id, int rssi, float snr);
  int peer_clock;

  unsigned long peer_next;
  unsigned long peer_refresh;

  double avg_peer_density;

  // char *fname;
  // File peer_log;

};

#endif
