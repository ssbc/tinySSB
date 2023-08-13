// ui_setup.h


LV_IMG_DECLARE(splash);
LV_IMG_DECLARE(tremola);

LV_IMG_DECLARE(chat);
LV_IMG_DECLARE(contacts);
LV_IMG_DECLARE(signal_btn);


//lv_obj_t *tSSB_ta;

lv_obj_t *btn_left;
lv_obj_t *bar;
lv_obj_t *btn_right;
lv_obj_t *posts; // grid layout
lv_obj_t *buttons;

// ---------------------------------------------------------------------------


void ui_setup()
{
    setupLvgl();

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
    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_task_handler();
    lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
    uint32_t run = millis() + 2000;
    while (millis() < run) {
        lv_task_handler();
        delay(5);
    }
    // lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del(img);

    // background
    
    img = lv_img_create(scr); // background
    lv_img_set_src(img, &tremola);
    lv_obj_set_pos(img, 0, 0);
    // lv_obj_set_pos(img, 1, 1); // deliberately off, we force 0,0 later
    lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_set_layout(posts, LV_LAYOUT_GRID);
    static lv_coord_t column_dsc[] = {314, LV_GRID_TEMPLATE_LAST};     // 1 column
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST}; // 3 rows
    lv_obj_set_grid_dsc_array(posts, column_dsc, row_dsc);

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 70);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 0, 1);
    lv_obj_t *lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e1");

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 110);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 1, 1);
    lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e2");

    tmp = lv_obj_create(posts);
    lv_obj_set_size(tmp, 314, 50);
    lv_obj_set_style_border_width(tmp, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(tmp, lv_color_hex(0xc0c0c0), LV_PART_MAIN);
    lv_obj_set_grid_cell(tmp, LV_GRID_ALIGN_START, 0, 1,
                         LV_GRID_ALIGN_START, 2, 1);
    lbl = lv_label_create(tmp);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0), LV_PART_MAIN);
    lv_label_set_text(lbl, "e3");

    // top row

    btn_left = lv_btn_create(img);
    lv_obj_set_size(btn_left, 30, 30);
    lv_obj_set_pos(btn_left, 3, 3);
    tmp = lv_label_create(btn_left);
    lv_label_set_text(tmp, "<");

    bar = lv_label_create(img);
    lv_obj_set_size(bar, LV_HOR_RES - 38 - 38, 30);
    lv_obj_set_pos(bar, 38, 3);
    lv_obj_add_style(bar, &bg_style, LV_PART_MAIN);
    lv_label_set_text(bar, "some title here\nand more on the next line");

    btn_right = lv_btn_create(img);
    lv_obj_set_size(btn_right, 30, 30);
    lv_obj_set_pos(btn_right, LV_HOR_RES - 33, 3);
    tmp = lv_label_create(btn_right);
    lv_label_set_text(tmp, ">");
    
    // four "tab" buttons

    lv_obj_t *ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 0, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, 0, &chat, 0);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 80, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, 0, &contacts, 0);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 160, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, 0, &signal_btn, 0);

    ib = lv_imgbtn_create(scr);
    lv_obj_set_style_pad_all(ib, 0, LV_PART_MAIN);
    lv_obj_set_size(ib, 80, 40);
    lv_obj_set_pos(ib, 240, LV_VER_RES - 40);
    lv_imgbtn_set_src(ib, LV_IMGBTN_STATE_RELEASED, 0, &chat, 0);

    /*
    // text area

    tSSB_ta = lv_textarea_create(scr);
    lv_obj_set_size(tSSB_ta, LV_HOR_RES-50, LV_VER_RES/3);
    lv_obj_center(tSSB_ta);
    // lv_obj_align(tSSB_ta, LV_ALIGN_CENTER, 0, 0);
    // lv_obj_set_pos(tSSB_ta, 0, 50);
    lv_obj_set_scrollbar_mode(tSSB_ta, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_style(tSSB_ta, &ta_bg_style, LV_PART_MAIN);
    lv_textarea_set_cursor_click_pos(tSSB_ta, true);
    // lv_textarea_set_cursor_hidden(tSSB_ta, false);
    lv_textarea_set_text_selection(tSSB_ta, false);
    lv_textarea_set_text(tSSB_ta, "");
    lv_textarea_set_max_length(tSSB_ta, 1024);

    lv_textarea_add_text(tSSB_ta, "\n>> tinySSB on T-Deck:\n\n     soon on this screen!\n");
    lv_textarea_set_cursor_pos(tSSB_ta, LV_TEXTAREA_CURSOR_LAST);
    */

    lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
}


// eof
