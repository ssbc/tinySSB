// app_tva.cpp

#include "config.h"

// #seq 1, pos=0

App_TVA_Class::App_TVA_Class(lv_obj_t *flex)
{
  this->flex = flex;
}


void App_TVA_Class::restream()
{
  Serial.println("\r\nRe-streaming all log entries:");
  for (int i = 0; i < theGOset->goset_len; i++) {
    unsigned char *fid = theGOset->get_key(i);
    ReplicaClass *r = theRepo->fid2replica(fid);

    // Serial.println();
    // r->hex_dump(0);

    int cnt = r->get_next_seq();
    for (int j = 1; j < cnt; j++) {
      int max_sz;
      int sz = r->get_content_len(j, &max_sz);
      if (sz < 0)
        continue;
      unsigned char *buf = r->read(j, &max_sz);
      if (buf == NULL)
        continue;
      // Serial.printf("  parsing seq=%d (%d bytes) - ", j, max_sz);
      struct bipf_s *b = bipf_loads(buf, max_sz);
      if (b != NULL) {
        Serial.println(bipf2String(b));
        // example: ["TAV","Hi Alice, all is fine. Best, Bob",false,1691097756]
        if (b->typ == BIPF_LIST &&  b->cnt >= 4) {
          struct bipf_s *app = b->u.list[0];
          if (app != NULL && app->typ == BIPF_STRING && !strncmp(app->u.str, "TAV", 3))
            new_post(fid, b);
        }
        bipf_free(b);
      } else {
        // Serial.printf("failed\r\n");
      }
      free(buf);
    }
  }
}


void App_TVA_Class::new_post(unsigned char *fid, struct bipf_s *tav)
{
  struct bipf_s *txt = tav->u.list[1], *voi = tav->u.list[2], *tim = tav->u.list[3];

  if (tim == NULL || tim->typ != BIPF_INT)
    return;
  long int when = tim->u.i;

  char *t;
  if (txt != NULL && txt->typ == BIPF_STRING)
    t = strndup(txt->u.str, txt->cnt);
  else if (voi != NULL && voi->typ == BIPF_BYTES)
    t = strdup("<voice>");
  else
    return;

  char *cp = ctime((time_t*)&when) + 4;
  cp[20] = '\0';
  char *msg = (char*) malloc(strlen(t) + 40);
  sprintf(msg, "%s:\n  %s\n%s", to_hex(fid, 6), t, cp);
  free(t);

  lv_obj_t *tmp = lv_obj_create(flex);
  lv_obj_set_size(tmp, 314, LV_SIZE_CONTENT);
  lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
  lv_obj_t *lbl = lv_label_create(tmp);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  lv_label_set_text(lbl, msg);
  lv_obj_scroll_to_view(tmp, LV_ANIM_ON);
  free(msg);

  uint32_t run = millis() + 50;
  while (millis() < run) {
    lv_task_handler();
    delay(5);
  }
}

// eof
