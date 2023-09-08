// app_tva.cpp

// (c) Aug 2023, <christian.tschudin@unibas.ch>

#if defined(TINYSSB_BOARD_TDECK)

#include "app_tva.h"

App_TVA_Class::App_TVA_Class(lv_obj_t *flex)
{
  this->flex = flex;
  post_cnt = 0;
  post_vect = NULL;
}


static int post_cmp(const void *aa, const void *bb)
{
  struct post_s *a = (struct post_s*) aa, *b = (struct post_s*) bb;
  return a->when - b->when;
}


void App_TVA_Class::restream()
{
  Serial.println("\r\n-- re-streaming all log entries");

  for (int i = 0; i < theGOset->goset_len; i++) {
    unsigned char *fid = theGOset->get_key(i);
    ReplicaClass *r = theRepo->fid2replica(fid);

    // Serial.println();
    // r->hex_dump(0);

    int cnt = r->get_next_seq();
    for (int j = 1; j < cnt; j++) {
      lv_task_handler();

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
        // Serial.println(bipf2String(b));
        _bipf2post(fid, b); // new_post(fid, b);
        bipf_free(b);
      } else {
        // Serial.printf("failed\r\n");
      }
      free(buf);
    }
  }

  if (post_cnt > 0) {
    qsort(post_vect, post_cnt, sizeof(struct post_s), post_cmp);
    struct post_s *p = post_vect;
    lv_obj_t *o;
    for (int i = 0; i < post_cnt; i++, p++) {
      o = _add_to_flex(p->txt);
      free(p->txt);
      p->txt = NULL;
    }
    lv_obj_scroll_to_view(o, LV_ANIM_ON);

    uint32_t run = millis() + 50;
    while (millis() < run) {
      lv_task_handler();
      delay(5);
    }
  }

  Serial.println();
}


struct post_s* App_TVA_Class::_bipf2post(unsigned char *fid, struct bipf_s *tav)
{
  // example: [ "TAV", "Hi Alice, all is fine. Best, Bob", false, 1691097756 ]
  if (tav->typ != BIPF_LIST ||  tav->cnt < 4)
    return NULL;
  struct bipf_s *app = tav->u.list[0];
  if (app->typ != BIPF_STRING || strncmp(app->u.str, "TAV", 3))
    return NULL;
  struct bipf_s *txt = tav->u.list[1], *voi = tav->u.list[2], *tim = tav->u.list[3];

  if (tim == NULL || tim->typ != BIPF_INT)
    return NULL;
  char *t;
  if (txt != NULL && txt->typ == BIPF_STRING)
    t = strndup(txt->u.str, txt->cnt);
  else if (voi != NULL && voi->typ == BIPF_BYTES)
    t = strdup("<voice>");
  else
    return NULL;

  post_cnt++;
  post_vect = (struct post_s*) realloc(post_vect, post_cnt * sizeof(struct post_s));

  struct post_s *p = post_vect + (post_cnt-1);
  p->txt = (char*) malloc(strlen(t) + 40);
  p->when = tim->u.i;

  char *cp = ctime((time_t*)&(p->when)) + 4;
  cp[20] = '\0';
  sprintf(p->txt, "%s:\n  %s\n%s", to_hex(fid, 6), t, cp);

  return p;
}


lv_obj_t* App_TVA_Class::_add_to_flex(char *txt)
{
  lv_obj_t *tmp = lv_obj_create(flex);

  lv_obj_set_size(tmp, 314, LV_SIZE_CONTENT);
  lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);

  lv_obj_t *lbl = lv_label_create(tmp);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  lv_label_set_text(lbl, txt);

  return tmp;
}


void App_TVA_Class::new_post(unsigned char *fid, struct bipf_s *tav)
{
  struct post_s *p = _bipf2post(fid, tav);
  if (p == NULL)
    return;
  
  lv_obj_t *e = _add_to_flex(p->txt);
  lv_obj_scroll_to_view(e, LV_ANIM_ON);

  // FIXME: bubble the entry to the right position, look at when values

  uint32_t run = millis() + 50;
  while (millis() < run) {
    lv_task_handler();
    delay(5);
  }
}

#endif // TINYSSB_BOARD_TDECK

// eof
