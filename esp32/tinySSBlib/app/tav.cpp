// tinySSBlib/app/tva.cpp

// (c) 2023-2025, <christian.tschudin@unibas.ch>

// T-Watch specific

// keeps a list of the 20 most recent posts (txt only),
// does device-specific filtering of the text field


#include "../tinySSBlib.h"

bool App_TAV_Class::publish_text(ReplicaClass *r, signing_fct s, char *txt)
{
  struct bipf_s *lst = bipf_mkList();
  bipf_list_append(lst, bipf_mkString("TAV"));
  bipf_list_append(lst, bipf_mkList()); // crossreferences
  bipf_list_append(lst, bipf_mkString(txt));
  bipf_list_append(lst, bipf_mkNone()); // voice
  time_t now;
  time(&now);
  bipf_list_append(lst, bipf_mkInt(now)); // timestamp

  int len = bipf_encodingLength(lst);
  unsigned char *buf = (unsigned char *) malloc(len);
  bipf_encode(buf, lst);

  bool rc = r->write(buf, len, s);

  bipf_free(lst);
  free(buf);

  return rc;
}

struct post_s* App_TAV_Class::incoming(unsigned char *fid,
                                       struct bipf_s *tav, int *pos)
{
  // example: [ "TAV", "Hi Alice, all is fine. Best, Bob", false, 1691097756 ]
  if (tav->typ != BIPF_LIST ||  tav->cnt < 4)
    return NULL;
  struct bipf_s *app = tav->u.list[0];
  if (app->typ != BIPF_STRING || strncmp(app->u.str, "TAV", 3))
    return NULL;
  struct bipf_s *txt = tav->u.list[2];
  // struct bipf_s *voi = tav->u.list[3];
  struct bipf_s *tim = tav->u.list[4];

  if (txt == NULL || txt->typ != BIPF_STRING ||
      tim == NULL || tim->typ != BIPF_INT)
    return NULL;
  int when = tim->u.i;

  struct post_s *p;
  if (cnt == 0) {
    cnt = 1;
    top20 = p = (struct post_s*) malloc(sizeof(struct post_s));
  } else if (cnt < APP_TAV_MAX_POSTS) {
    cnt++;
    top20 = (struct post_s*) realloc(top20, cnt * sizeof(struct post_s));
    p = top20;
    int ndx = 0;
    while (ndx < (cnt-1) && when > p->when) {
      p++, ndx++;
    }
    if (ndx < (cnt-1)) // new msg is not earliest
      memmove(p+1, p, (cnt - 1 - ndx) * sizeof(struct post_s));
  } else { // vector is full: find where new post belongs, drop oldest post
    p = top20;
    int ndx = 0;
    while (ndx < APP_TAV_MAX_POSTS && when > p->when) {
      p++, ndx++;
    }
    if (ndx == 0) // new msg is older than oldest, quit
      return NULL;
    free(top20->txt); // release oldest msg
    memmove(top20, top20 + 1, p - (top20 + 1));
  }
  p->txt = strndup(txt->u.str, txt->cnt);
#ifdef TINYSSB_BOARD_TWATCH
    char *cp = strstr(p->txt, "data:image/svg+bipf;base64");
    if (cp)
      strcpy(cp, "<sketch>");
    int offs = 0;
    if (!strncmp(p->txt, "pfx:loc", 7)) {
      offs = 9;
      strcpy(p->txt + offs, "loc");
      p->txt[offs + 3] = '=';
      p->txt[24] = '\n';
    }
    cp = p->txt + offs +strlen(p->txt + offs) - 1;
    while ((*cp == ' ' || *cp == '\n') && cp > (p->txt + offs)) // trim
      *(cp--) = '\0';
    if (offs > 0) {
      cp = strdup(p->txt + offs);
      free(p->txt);
      p->txt = cp;
    }
#endif
  p->when = when;
  p->aux = NULL;

  if (pos != NULL)
    *pos = p - top20;

  theUI->add_new_post(p->txt, p->when, p - top20);
  
  return p;
}


void App_TAV_Class::restream(void *frontier)
{
  for (int i = 0; i < theGOset->goset_len; i++) {
    // Serial.println("l1");
    unsigned char *fid = theGOset->get_key(i);
    ReplicaClass *r = theRepo->fid2replica(fid);

    int len = r->get_next_seq();
    for (int j = 1; j < len; j++) {
      // Serial.println("l2");
      int max_sz;
      int sz = r->get_content_len(j, &max_sz);
      if (sz < 0 || sz > 2024 || max_sz != sz) // only consider full entries
        continue;
      // Serial.printf("l2b - seq=%d, len=%d\r\n", j, max_sz);
      unsigned char *buf = r->read(j, &max_sz);
      if (buf == NULL)
        continue;
      // Serial.printf("  parsing seq=%d (%d bytes) - ", j, max_sz);
      // Serial.printf("  %s\r\n", to_hex(buf, min(max_sz,64)));
      // Serial.println("l2c");
      struct bipf_s *b = bipf_loads(buf, max_sz);
      if (b != NULL) {
        // Serial.println(bipf2String(b));
        incoming(fid, b);
        // Serial.println("l2d");
        bipf_free(b);
      } else {
        // Serial.printf("  failed parsing seq=%d\r\n", j);
      }
      free(buf);
      theUI->loop();
    }
  }
}

/*
static int post_cmp(const void *aa, const void *bb)
{
  struct post_s *a = (struct post_s*) aa;
  struct post_s *b = (struct post_s*) bb;
  return a->when - b->when;
}

lv_obj_t* App_TAV_Class::_add_to_flex(char *txt)
{
  lv_obj_t *tmp = lv_obj_create(flex);

  lv_obj_set_size(tmp, 314, LV_SIZE_CONTENT);
  lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);

  lv_obj_t *lbl = lv_label_create(tmp);
  lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  lv_label_set_text(lbl, txt);

  return tmp;
}


void App_TAV_Class::new_post(unsigned char *fid, struct bipf_s *tav)
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
*/

// eof
