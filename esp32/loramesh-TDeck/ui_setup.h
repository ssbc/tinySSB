// ui_setup.h


LV_IMG_DECLARE(splash);
LV_IMG_DECLARE(tremola);

LV_IMG_DECLARE(chat);
LV_IMG_DECLARE(chat_checked);
LV_IMG_DECLARE(contacts);
LV_IMG_DECLARE(contacts_checked);
LV_IMG_DECLARE(haze0);
LV_IMG_DECLARE(haze1);
LV_IMG_DECLARE(haze2);
LV_IMG_DECLARE(menu);
LV_IMG_DECLARE(return_btn);
LV_IMG_DECLARE(signal_btn);
LV_IMG_DECLARE(signal_btn_checked);
LV_IMG_DECLARE(tab_left);
LV_IMG_DECLARE(tab_right);


lv_obj_t *tSSB_ta;

lv_obj_t *btn_left;
lv_obj_t *bar;
lv_obj_t *btn_right;
lv_obj_t *posts; // flex layout
lv_obj_t *log_lbl;
lv_obj_t *four_buttons[4], *current_btn;

lv_obj_t *hz[3];

// ---------------------------------------------------------------------------

void prep2log(char *txt)
{
  char *olds = lv_label_get_text(log_lbl);
  char *news = (char*) malloc(strlen(olds) + strlen(txt) + 8);
  strcpy(news, txt);
  strcpy(news + strlen(news), "\n---\n");
  strcpy(news + strlen(news), olds);
  int i = strlen(news);
  if (i > 1024)
    news[1024] = '\0';
  lv_label_set_text(log_lbl, news);
  free(news);
  lv_obj_invalidate(log_lbl);
}

// extern lv_imgbtn_state_t get_state(const lv_obj_t * imgbtn);

void brightness_cb(lv_event_t *e)
{
  static char lvl;

  lvl = (lvl + 1) % 4;
  if (lvl == 0) {
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hz[2], LV_OBJ_FLAG_HIDDEN);
  } else if (lvl == 1) {
    lv_obj_clear_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
  } else if (lvl == 2) {
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hz[2], LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_invalidate(hz[0]);
  lv_obj_invalidate(hz[1]);
  lv_obj_invalidate(hz[2]);
}


void four_button_cb(lv_event_t *e)
{
  lv_obj_t *t = lv_event_get_target(e);
  // void *aux = lv_event_get_user_data(e);

  if (current_btn == t)
    return;

  // lv_event_stop_bubbling(e);
  for (int i = 0; i < 4; i++) {
    lv_obj_clear_state(four_buttons[i],
                       LV_STATE_CHECKED | LV_STATE_PRESSED);
    if (t == four_buttons[i])
      current_btn = t;
    else
      lv_obj_add_state(four_buttons[i], LV_STATE_CHECKED);
    lv_obj_invalidate(four_buttons[i]);
  }
  lv_task_handler();
}


void ui_setup()
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_t *tmp;

    /*
    static lv_style_t zeropad_style;
    lv_style_set_pad_all(&zeropad_style, 0);
    lv_obj_remove_style_all(scr);
    lv_obj_add_style(scr, &zeropad_style, LV_PART_MAIN);
    
    static lv_style_t lable_style;
    lv_style_init(&lable_style);
    lv_style_set_bg_color(&lable_style, lv_color_white());
    lv_style_set_text_color(&lable_style, lv_color_black());

    static lv_style_t bg_style;
    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_style_set_text_color(&bg_style, lv_color_hex(0x8080ff));

    static lv_style_t btn_bg_style;
    lv_style_init(&btn_bg_style);
    lv_style_set_bg_color(&btn_bg_style, lv_color_hex(0xc0c0ff));
    */

    static lv_style_t bg_style;
    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_style_set_text_color(&bg_style, lv_color_hex(0xff4040));

    static lv_style_t ta_bg_style;
    lv_style_init(&ta_bg_style);
    lv_style_set_bg_color(&ta_bg_style, lv_color_white());
    lv_style_set_text_color(&ta_bg_style, lv_color_hex(0x8080ff));
    // lv_style_set_bg_opa(&ta_bg_style, LV_OPA_100);

    // splash

    lv_obj_t *img = lv_img_create(scr); // background
    lv_img_set_src(img, &splash);
    lv_obj_set_size(img, 320, 240);
    lv_obj_set_pos(img, 0, 0);
    
    lv_obj_invalidate(img);
    uint32_t run = millis() + 1000;
    while (millis() < run) {
        lv_task_handler();
        delay(5);
    }
    lv_obj_del(img);

    // background
    
    img = lv_img_create(scr); // background
    lv_img_set_src(img, &tremola);
    lv_obj_set_pos(img, 0, 0);
    lv_obj_invalidate(img);
    lv_task_handler();

    // posts (and scroll)

    // lv_obj_t *scroll = lv_obj_create(img);
    posts = lv_obj_create(img);
    lv_obj_set_size(posts, 314, 160);
    lv_obj_set_pos(posts, 3, 36);
    lv_obj_set_style_pad_all(posts, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(posts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(posts, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(posts, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(posts, LV_FLEX_FLOW_COLUMN);
    /*
    static lv_coord_t column_dsc[] = {314, LV_GRID_TEMPLATE_LAST};     // 1 column
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}; // 4 rows
    lv_obj_set_grid_dsc_array(posts, column_dsc, row_dsc);
    */
    // text area

    /*
    tSSB_ta = lv_textarea_create(posts);
    lv_obj_set_size(tSSB_ta, 314, 70);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(tSSB_ta, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(tSSB_ta, &ta_bg_style, LV_PART_MAIN);
    lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 0, 1);
    lv_textarea_set_cursor_click_pos(tSSB_ta, true);
    // lv_textarea_set_cursor_hidden(tSSB_ta, false);
    lv_textarea_set_text_selection(tSSB_ta, false);
    lv_textarea_set_text(tSSB_ta, "");
    lv_textarea_set_max_length(tSSB_ta, 1024);

    lv_textarea_add_text(tSSB_ta, "\n>> tinySSB on T-Deck:\n\n     soon on this screen!\n");
    lv_textarea_set_cursor_pos(tSSB_ta, LV_TEXTAREA_CURSOR_LAST);
    */

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, LV_SIZE_CONTENT);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    // lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
    //                      LV_GRID_ALIGN_START, 0, 1);
    log_lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(log_lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_long_mode(log_lbl, LV_LABEL_LONG_WRAP);
    lv_label_set_text(log_lbl, "log");
    
    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 70);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    // lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
    //                      LV_GRID_ALIGN_START, 1, 1);
    lv_obj_t *lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e1");

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 110);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    // lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
    //                      LV_GRID_ALIGN_START, 2, 1);
    lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e2");

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 50);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    // lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
    //                     LV_GRID_ALIGN_START, 3, 1);
    lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e3 - extended");

    // top row (left and right button, middle title)

    btn_left = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(btn_left, 0, LV_PART_MAIN);
    lv_obj_set_size(btn_left, 30, 30);
    lv_obj_set_pos(btn_left, 3, 3);
    lv_imgbtn_set_src(btn_left, LV_IMGBTN_STATE_RELEASED, 0, &return_btn, 0);
    lv_obj_add_event_cb(btn_left, brightness_cb, LV_EVENT_CLICKED, NULL);

    bar = lv_label_create(img);
    lv_obj_set_size(bar, LV_HOR_RES - 38 - 38, 30);
    lv_obj_set_pos(bar, 38, 3);
    lv_obj_add_style(bar, &bg_style, LV_PART_MAIN);
    lv_label_set_text(bar, "some title here\nand more on the next line");

    btn_right = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(btn_right, 0, LV_PART_MAIN);
    lv_obj_set_size(btn_right, 30, 30);
    lv_obj_set_pos(btn_right, LV_HOR_RES - 33, 3);
    lv_imgbtn_set_src(btn_right, LV_IMGBTN_STATE_RELEASED, 0, &menu, 0);
    
    // four "tab" buttons

    lv_obj_t *ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 0, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &chat, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &chat_checked, 0);
    four_buttons[0] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 80, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &contacts, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &contacts_checked, 0);
    four_buttons[1] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 160, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &signal_btn, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &signal_btn_checked, 0);
    four_buttons[2] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 240, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, &tab_left, &chat, &tab_right);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_CHECKED_RELEASED, 0, &chat_checked, 0);
    four_buttons[3] = ib;
    lv_obj_add_event_cb(ib, four_button_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(ib, LV_STATE_CHECKED);

    hz[0] = lv_img_create(scr); // haze
    lv_img_set_src(hz[0], &haze0);
    lv_obj_set_size(hz[0], 320, 240);
    lv_obj_set_pos(hz[0], 0, 0);
    lv_obj_add_flag(hz[0], LV_OBJ_FLAG_HIDDEN);
// lv_canvas_fill_bg(hz[0], lv_color_hex(0), 32);

    hz[1] = lv_img_create(scr); // haze
    lv_img_set_src(hz[1], &haze1);
    lv_obj_set_size(hz[1], 320, 240);
    lv_obj_set_pos(hz[1], 0, 0);
    lv_obj_add_flag(hz[1], LV_OBJ_FLAG_HIDDEN);
    // lv_canvas_fill_bg(hz[1], lv_color_hex(0), 32);

    hz[2] = lv_img_create(scr); // haze
    lv_img_set_src(hz[2], &haze2);
    lv_obj_set_size(hz[2], 320, 240);
    lv_obj_set_pos(hz[2], 0, 0);
    lv_obj_add_flag(hz[2], LV_OBJ_FLAG_HIDDEN);
    
    /*
    hz = lv_canvas_create(scr);
    lv_obj_set_size(hz, 320, 240);
    lv_obj_set_pos(hz, 0, 0);
    void *buf = lv_mem_alloc(lv_img_cf_get_px_size(LV_IMG_CF_INDEXED_4BIT * 320) / 8 * 240);
    lv_canvas_set_buffer(haze, buf, 320, 240, LV_IMG_CF_INDEXED_4BIT);
    for (int i = 0; i < 16; i++)
      lv_canvas_set_palette(haze, i, lv_color_hex(i * 0x111111));
    // lv_canvas_fill_bg(haze, lv_color_hex(0xc), 128);
    */
    
    lv_obj_invalidate(scr); // background
    lv_task_handler();
}


// eof
