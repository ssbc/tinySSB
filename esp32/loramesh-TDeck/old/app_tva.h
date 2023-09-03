// app_tva.h

#include "config.h"

struct post_s {
  char *txt;
  long when;
};


class App_TVA_Class {

public:
  App_TVA_Class(lv_obj_t *flex);

  void restream();
  void new_post(unsigned char *fid, struct bipf_s *msg);

private:
  lv_obj_t* _add_to_flex(char *txt);
  struct post_s* _bipf2post(unsigned char *fid, struct bipf_s *b);
  int post_cnt;
  struct post_s *post_vect;
  lv_obj_t *flex; // our widget
};

extern App_TVA_Class *the_TVA_app;

// eof
