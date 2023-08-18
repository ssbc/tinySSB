// app_tva.h

#include "config.h"

class App_TVA_Class {

public:
  App_TVA_Class(lv_obj_t *flex);

  void restream();
  void new_post(unsigned char *fid, struct bipf_s *msg);

private:
  int cnt;
  long *timestamps;
  lv_obj_t *flex;

};

extern App_TVA_Class *thePosts;

// eof
