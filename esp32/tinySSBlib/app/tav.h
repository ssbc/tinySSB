// tinySSBlib/app/tva.h

// specific to the T-Watch

// extract ASCII and timestamp from public chat msgs (posts),
// keep the 20 most recent posts

struct post_s {
  char *txt; // string is stored on heap
  int when;  // int32 - timestamp (sec since Jan 1, 1970)
  void *aux; // can be used by UI
};

#define APP_TAV_MAX_POSTS 20

class App_TAV_Class {

public:
  // create new post message, append to the log
  bool publish_text(ReplicaClass *r, signing_fct s, char *text);
  
  // check whether given log entry is a TAV entry, insert into ordered vector
  struct post_s* incoming(unsigned char *fid,
                          struct bipf_s *msg, int *pos = NULL);

  // loop over the whole repo  
  void restream(void *frontier = NULL);

  int cnt = 0;
  struct post_s *top20 = NULL; // sorted vector of posts, oldest first
};

// eof
