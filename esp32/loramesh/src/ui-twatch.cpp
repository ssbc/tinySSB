// ui-twatch.cpp

#include "ui-twatch.h"

#if defined(TINYSSB_BOARD_TWATCH)

#include "lib/tinySSBlib.h"

#include "hardware.h"
#include "lib/cmd.h"

#include "gesture/dollar-q.h"

#include "mbedtls/base64.h"
#include "ArduinoNvs.h"

#include <SPI.h>
#include <TFT_eSPI.h>
#include <Wire.h>
// #include "es7210.h"
// #include <Audio.h>
// #include <driver/i2s.h>

extern TFT_eSPI    tft;
extern void setBrightness(uint8_t level);

#include <TouchDrvFT6X36.hpp>
extern TouchDrvFT6X36 sensor_touch;

extern char mute_io;

// ---------------------------------------------------------------------------

static bool mk_signature(unsigned char *sig, unsigned char *data, int len)
{
  UI_TWatch_Class *watch = (UI_TWatch_Class*) theUI;
  if (!watch->myid_valid)
    return false;
  crypto_sign_ed25519_detached(sig, NULL, data, len, watch->myid_sk);
  return true;
}

static void touchpad_read2(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int16_t x, y;
    bool touched = sensor_touch.getPoint(&x, &y);
    if ( !touched ) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        switch (tft.getRotation()) {
        case 0:
            data->point.x = TFT_WIDTH - x;
            data->point.y = TFT_HEIGHT - y;
            break;
        case 1:
            data->point.x = TFT_WIDTH - y;
            data->point.y =  x;
            break;
        case 3:
            data->point.x = y;
            data->point.y = TFT_HEIGHT - x;
            break;
        case 2:
        default:
            data->point.x = x;
            data->point.y = y;
        }
        // Serial.printf("# touch X:%d Y:%d\r\n", data->point.x, data->point.y);
        data->state = LV_INDEV_STATE_PR;
    }
}

// ---------------------------------------------------------------------------

SemaphoreHandle_t xSemaphore = NULL;

static void disp_flush( lv_disp_drv_t *disp,
                        const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    if ( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
        tft.startWrite();
        tft.setAddrWindow( area->x1, area->y1, w, h );
        tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
        tft.endWrite();
        lv_disp_flush_ready( disp );
        xSemaphoreGive( xSemaphore );
    }
}

static void setupLvgl()
{
  Serial.println("# setupLvgl()");
    static lv_disp_draw_buf_t draw_buf;

#ifndef BOARD_HAS_PSRAM
#define LVGL_BUFFER_SIZE    ( TFT_WIDTH * 50 )
    static lv_color_t buf1[ LVGL_BUFFER_SIZE ];
    static lv_color_t buf2[ LVGL_BUFFER_SIZE ];
#else
#define LVGL_BUFFER_SIZE    (TFT_WIDTH * TFT_HEIGHT * sizeof(lv_color_t))
    static lv_color_t *buf1 = (lv_color_t *) ps_malloc(LVGL_BUFFER_SIZE);
    static lv_color_t *buf2 = (lv_color_t *) ps_malloc(LVGL_BUFFER_SIZE);
    if (!buf1 || !buf2) {
      Serial.printf("PSRAM malloc for LVGL failed: free=%d %d %d %d, trying standard RAM\r\n",
                    ESP.getFreeHeap(),
                    LVGL_BUFFER_SIZE, TFT_WIDTH, TFT_HEIGHT);
      buf1 = (lv_color_t *) malloc(LVGL_BUFFER_SIZE);
      buf2 = (lv_color_t *) malloc(LVGL_BUFFER_SIZE);
      delay(5000);
      assert(buf1);
      assert(buf2);
    }
#endif
    // memset(buf, 0, LVGL_BUFFER_SIZE);

    String LVGL_Arduino = String("# LVLG Arduino ") + lv_version_major() +
                          "." + lv_version_minor() + "." + lv_version_patch();
    Serial.println( LVGL_Arduino );

    lv_init();
    lv_group_set_default(lv_group_create());

#ifndef BOARD_HAS_PSRAM
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, sizeof(buf1));
#else
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LVGL_BUFFER_SIZE);
#endif

    // Initialize the display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );

    disp_drv.hor_res = TFT_HEIGHT;
    disp_drv.ver_res = TFT_WIDTH;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef BOARD_HAS_PSRAM
    disp_drv.full_refresh = 1;
#endif
    lv_disp_drv_register( &disp_drv );

    // Initialize the input device driver
    static lv_indev_drv_t indev_touchpad;
    lv_indev_drv_init( &indev_touchpad );
    indev_touchpad.type = LV_INDEV_TYPE_POINTER;
    indev_touchpad.read_cb = touchpad_read2;
    lv_indev_drv_register( &indev_touchpad );
}

// ---------------------------------------------------------------------------
void graffiti_set_pixel(int x, int y);
void graffiti_clear();

UIClass::UIClass()
{
  Serial.println("# init of UIClass()");

  //Add mutex to allow multitasking access
  xSemaphore = xSemaphoreCreateBinary();
  assert(xSemaphore);
  xSemaphoreGive( xSemaphore );

  setupLvgl();

}

void UI_TWatch_Class::spinner(bool show)
{
  if (show)
    lv_obj_clear_flag(spin, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(spin, LV_OBJ_FLAG_HIDDEN);
  lv_task_handler();
}

void UI_TWatch_Class::loop()
{
  lv_task_handler();
}

// ---------------------------------------------------------------------------

extern UIClass *theUI;

#include "../assets/twatch/tremola.h"
#include "../assets/twatch/splash.h"

void UI_TWatch_Class::refresh()
{
  lv_obj_invalidate( lv_scr_act() );
  lv_task_handler();
}

// ---------------------------------------------------------------------------

#include "../assets/twatch/tinySSB_logo_240x132.h"

#include "../assets/twatch/clock_ticks3.h"
#include "../assets/twatch/clock_hour_hand.h"   //  20x150
#include "../assets/twatch/clock_minute_hand.h" //  20x235
#include "../assets/twatch/clock_second_hand.h" //  40x200

#define MAX_TEXT_LEN 64

static uint8_t pageId = 0;
static lv_obj_t *graffiti; // canvas
static char graffiti_text[MAX_TEXT_LEN+1];

static lv_style_t bgTransparent;
static lv_style_t tile_title_style;
static lv_style_t tile_text_style;
static lv_style_t graffiti_text_style;
static lv_style_t post_text_style;
static lv_style_t peers_title_style;
static lv_style_t peers_text_style;
static lv_style_t config_text_style;
static lv_style_t bg_grey_style;
static lv_style_t bg_green_style;
static lv_style_t bg_red_style;

static lv_obj_t *tileview;
static lv_obj_t *tile_clock;
static lv_obj_t *tile_chat;
static lv_obj_t *tile_chat_list;
static lv_obj_t *tile_chat_graffiti;
static lv_obj_t *tile_peers_list;
static lv_obj_t *tile_config_list;

static lv_obj_t *logo_img;
static lv_obj_t *hour_img;
static lv_obj_t *min_img;
static lv_obj_t *sec_img;

// graffiti objects (buttons):
static lv_obj_t *g_abort;
static lv_obj_t *g_text; // label
static lv_obj_t *g_ok;
static lv_obj_t *g_left;
static lv_obj_t *g_right;
static lv_obj_t *g_del;

// ---------------------------------------------------------------------------

/*

Set an event function for the container and in LV_EVENT_PRESSING

lv_indev_t * indev= lv_indev_create();
lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
lv_indev_set_mode(indev, LV_INDEV_MODE_EVENT);

lv_indev_set_read_cb(indev, read_cb);

in event handler:
    lv_indev_t * indev = lv_indev_get_act();
    lv_point_t p;
    lv_indev_get_point(indev, &p);

*/

lv_timer_t *tim = NULL;
short _x[1000];
short _y[1000];
short _sid[100];
struct gest_s *gest;
int pnt_cnt;
int sid = 0;

void cb_timer(lv_timer_t *t)
{
  // Serial.println("# cb_timer for gesture capturing");
  mute_io = false;

  if (pnt_cnt == 1) { // at least 2 points needed
    _x[1] = _x[0];
    _y[1] = _y[0];
    _sid[1] = _sid[0];
    pnt_cnt++;
  }

  /*
  for (int i = 0; i < pnt_cnt; i++)
    Serial.printf("pts %d %d %d\r\n", _x[i], _y[i], _sid[i]);
  */

  struct gest_s *g = qd_import(_x, _y, _sid, pnt_cnt);
  /*
  for (int i = 0; i < g->cnt; i++) {
    uint32_t pt = g->pts[i];
    Serial.printf("_p_ %d %d %d\r\n", pt_x(pt), pt_y(pt), pt_sid(pt));
  }
  */
  char X = qd_classify(g);
  Serial.printf("# retrieved gesture for '%c' (%d)\r\n", X, X);
  qd_free(g);
  int pos = strlen(graffiti_text);
  if (pos < sizeof(graffiti_text)-2) {
    graffiti_text[pos] = X;
    graffiti_text[pos+1] = '\0';
    lv_label_set_text(g_text, graffiti_text);
  }
  graffiti_clear();

  pnt_cnt = 0;
  sid = 0;
  tim = NULL;
}

static void cb_g_draw(lv_event_t *e)
{
  lv_obj_t *target = lv_event_get_current_target(e);
  if (target != tile_chat_graffiti)
    return;
  int c = lv_event_get_code(e);
  lv_indev_t * indev = lv_indev_get_act();
  lv_point_t p;
  lv_indev_get_point(indev, &p);

  // printf("pressing %d,%d, code %d\n", p.x, p.y, c);

  graffiti_set_pixel(p.x, p.y);

  _x[pnt_cnt] = p.x;
  _y[pnt_cnt] = 240 - p.y;
  _sid[pnt_cnt] = sid;
  pnt_cnt++;
  if (c == 2) { // start
    mute_io = true;
    if (tim) {
      lv_timer_del(tim);
      tim = NULL;
    }
  } else if (c == 8) { // stop
    tim = lv_timer_create(cb_timer, 500, NULL);
    lv_timer_set_repeat_count(tim, 1);
    if (pnt_cnt == 1 || _sid[pnt_cnt-2] != sid) { // at least 2 pts per seg
      _x[pnt_cnt] = p.x;
      _y[pnt_cnt] = 240 - p.y;
      _sid[pnt_cnt] = sid;
      pnt_cnt++;
    }
    sid++;
  }
}

static void cb_g_btn(lv_event_t * e)
{
  lv_obj_t *target = lv_event_get_current_target(e);
  if (target == g_abort)
    lv_obj_add_flag(tile_chat_graffiti, LV_OBJ_FLAG_HIDDEN);
  else if (target == g_del) {
    if (strlen(graffiti_text) > 0) {
      graffiti_text[strlen(graffiti_text)-1] = '\0';
      lv_label_set_text(g_text, graffiti_text);
    }
  } else if (target == g_left) {
    strcpy(graffiti_text, "no ");
    lv_label_set_text(g_text, graffiti_text);
  } else if (target == g_right) {
    strcpy(graffiti_text, "yes ");
    lv_label_set_text(g_text, graffiti_text);
  } else if (target == g_ok) {
    // Serial.printf("ok <%s><%s>\r\n", graffiti_text, lv_label_get_text(g_text));
    if (strlen(graffiti_text) > 0) {
      ReplicaClass *r = theRepo->fid2replica(((UI_TWatch_Class*) theUI)->myid_pk);
      theTAV->publish_text(r, ((UI_TWatch_Class*) theUI)->my_signing_fct,
                           graffiti_text);
      graffiti_text[0] = '\0';
      lv_label_set_text(g_text, "");
    }
    lv_obj_add_flag(tile_chat_graffiti, LV_OBJ_FLAG_HIDDEN);
  }
}

void graffiti_clear()
{
    lv_color_t c;
    c.full = 0;
    lv_canvas_fill_bg(graffiti, c, LV_OPA_COVER);
}

void graffiti_set_pixel(int x, int y)
{
    lv_color_t c;
    c.full = 1;
    lv_canvas_set_px_color(graffiti, x-1, y-1, c);
    lv_canvas_set_px_color(graffiti, x+0, y-1, c);
    lv_canvas_set_px_color(graffiti, x+1, y-1, c);
    lv_canvas_set_px_color(graffiti, x-1, y+0, c);
    lv_canvas_set_px_color(graffiti, x+0, y+0, c);
    lv_canvas_set_px_color(graffiti, x+1, y+0, c);
    lv_canvas_set_px_color(graffiti, x-1, y+1, c);
    lv_canvas_set_px_color(graffiti, x+0, y+1, c);
    lv_canvas_set_px_color(graffiti, x+1, y+1, c);
}

lv_obj_t* mk_graffiti(lv_obj_t *tile)
{
    lv_style_init(&graffiti_text_style);
    // lv_style_set_text_decor(&peers_title_style, LV_TEXT_DECOR_UNDERLINE);
    lv_style_set_bg_color(&graffiti_text_style, lv_color_hex(0xc0c0c0));
    lv_style_init(&graffiti_text_style);
    lv_style_set_bg_color(&graffiti_text_style, lv_color_hex(0xe0e0e0));

    lv_obj_t *g = lv_obj_create(tile), *tmp;
    lv_obj_set_style_pad_all(g, 0, 0);
    lv_obj_set_style_border_width(g, 0, LV_PART_MAIN);
    lv_obj_set_size(g, 240, 240);
    lv_obj_set_pos(g, 0, 0);
    lv_obj_add_flag(g, LV_OBJ_FLAG_HIDDEN);

    lv_obj_clear_flag(g, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);

    graffiti = lv_canvas_create(g);
    lv_obj_set_style_pad_all(graffiti, 0, 0);
    lv_obj_set_style_border_width(graffiti, 0, LV_PART_MAIN);
    lv_obj_clear_flag(graffiti, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_set_pos(graffiti, 0, 0);
    lv_obj_set_size(graffiti, 240, 240);
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_INDEXED_1BIT(240-2,240-2)];
    lv_canvas_set_buffer(graffiti, cbuf, 240-2, 240-2, LV_IMG_CF_INDEXED_1BIT);
    lv_canvas_set_palette(graffiti, 0, lv_color_hex(0xd0d0d0));
    lv_canvas_set_palette(graffiti, 1, lv_color_hex(0xff4040));

    lv_color_t c;
    c.full = 0;
    lv_canvas_fill_bg(graffiti, c, LV_OPA_COVER);
 
    g_abort = lv_btn_create(g);     lv_obj_set_size(g_abort, 40,  40);
    g_ok    = lv_btn_create(g);     lv_obj_set_size(g_ok,    50,  40);
    g_left  = lv_btn_create(g);     lv_obj_set_size(g_left,  40,  50);
    g_right = lv_btn_create(g);     lv_obj_set_size(g_right, 40,  50);
    g_del   = lv_btn_create(g);     lv_obj_set_size(g_del,   40,  75);
    tmp     = lv_obj_create(g);     lv_obj_set_size(tmp,    134,  40);
    g_text  = lv_label_create(tmp); lv_obj_set_size(g_text, 125,  30);

    lv_obj_add_style(g_abort, &bg_red_style, LV_PART_MAIN);
    lv_obj_add_style(g_ok,    &bg_green_style, LV_PART_MAIN);
    lv_obj_add_style(tmp,     &bg_grey_style, LV_PART_MAIN);
    lv_obj_add_style(g_text,  &bg_grey_style, LV_PART_MAIN);
    lv_obj_add_style(g_text,  &graffiti_text_style, LV_PART_MAIN);
    lv_obj_set_style_text_align(g_text, LV_TEXT_ALIGN_RIGHT, 0);
    // lv_label_set_long_mode(g_text, LV_LABEL_LONG_SCROLL);
    lv_label_set_long_mode(g_text, LV_LABEL_LONG_CLIP);

    lv_obj_set_pos(g_left,    2,   2);
    lv_obj_set_pos(g_del,     2,  58);
    lv_obj_set_pos(g_right,   2, 138);
    lv_obj_set_pos(g_abort,   2, 194);
    lv_obj_set_pos(tmp,      46, 194);
    lv_obj_set_pos(g_text,    5,  10);
    lv_obj_set_pos(g_ok,    184, 194);

    lv_label_set_text(g_text, "");
    lv_obj_t *label;
    label = lv_label_create(g_abort); lv_label_set_text(label, "X");
    label = lv_label_create(g_ok);    lv_label_set_text(label, "OK");
    label = lv_label_create(g_left);  lv_label_set_text(label, "<");
    label = lv_label_create(g_right); lv_label_set_text(label, ">");
    label = lv_label_create(g_del);   lv_label_set_text(label, "D\nE\nL");

    lv_obj_add_event_cb(g_abort, cb_g_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(g_ok,    cb_g_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(g_left,  cb_g_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(g_right, cb_g_btn, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(g_del,   cb_g_btn, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(g, cb_g_draw, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(g, cb_g_draw, LV_EVENT_RELEASED, NULL);

    return g;
}

void cb_new_post(lv_event_t *e)
{
  lv_obj_clear_flag(tile_chat_graffiti, LV_OBJ_FLAG_HIDDEN);
  graffiti_text[0] = '\0';
}

void mk_tile_chat(lv_obj_t *tile)
{
  lv_obj_t *label;

  lv_obj_set_style_pad_all(tile, 0, 0);
  lv_obj_add_flag(tile, LV_OBJ_FLAG_HIDDEN); // while initializing

  tile_chat_list = lv_obj_create(tile);
  // lv_obj_add_flag(tile_chat_list, LV_OBJ_FLAG_HIDDEN); // while initializing
  lv_obj_set_style_pad_all(tile_chat_list, 0, 0);
  lv_obj_set_size(tile_chat_list, 240, 240);
  lv_obj_set_pos(tile_chat_list, 0, 0);
  lv_obj_add_style(tile_chat_list, &bgTransparent, LV_PART_MAIN);

  lv_obj_t *btn = lv_btn_create(tile);
  lv_obj_set_size(btn, 40, 40);
  lv_obj_set_pos(btn, 180, 20);
  lv_obj_add_event_cb(btn, cb_new_post, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(btn);
  lv_label_set_text(label, "+");

  tile_chat_graffiti = mk_graffiti(tile);
  
  lv_style_init(&post_text_style);
  lv_style_set_text_color(&post_text_style, lv_color_black());
  lv_style_set_bg_opa(&post_text_style, LV_OPA_COVER);
 
  lv_obj_set_layout(tile_chat_list, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(tile_chat_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(tile_chat_list, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(tile_chat_list, 10, LV_PART_MAIN);

  label = lv_label_create(tile_chat_list);
  lv_obj_add_style(label, &tile_title_style, LV_PART_MAIN);
  lv_label_set_text(label, "Chat");
  lv_obj_set_style_pad_bottom(label, 15, 0);
}

// ---------------------------------------------------------------------------
extern void update_peers();
extern void update_config();

void update_hand()
{
    if (pageId != 0)
        return;

    time_t now;
    struct tm  timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    lv_img_set_angle(hour_img,
                  ((timeinfo.tm_hour) * 300 + ((timeinfo.tm_min) * 5)) % 3600);
    lv_img_set_angle(min_img, (timeinfo.tm_min) * 60);
    lv_img_set_angle(sec_img, (timeinfo.tm_sec) * 60);
}

void mk_tile_clock(lv_obj_t *parent)
{
    bool antialias = true;

    lv_obj_t *ticks_img = lv_img_create(parent);
    lv_img_set_src(ticks_img, &clock_ticks3);
    lv_obj_center(ticks_img);
    lv_img_set_antialias(ticks_img, antialias);

    hour_img = lv_img_create(parent);
    lv_img_set_src(hour_img, &clock_hour_hand);
    lv_obj_center(hour_img);
    lv_img_set_pivot(hour_img,
                     clock_hour_hand.header.w / 2,
                     clock_hour_hand.header.h / 2);
    lv_img_set_antialias(hour_img, antialias);

    min_img = lv_img_create(parent);
    lv_img_set_src(min_img,  &clock_minute_hand);
    lv_obj_center(min_img);
    lv_img_set_pivot(min_img,
                     clock_minute_hand.header.w / 2,
                     clock_minute_hand.header.h / 2);
    lv_img_set_antialias(min_img, antialias);

    sec_img = lv_img_create(parent);
    lv_img_set_src(sec_img,  &clock_second_hand);
    lv_obj_center(sec_img);
    lv_img_set_pivot(sec_img,
                     clock_second_hand.header.w / 2,
                     clock_second_hand.header.h / 2);
    lv_img_set_antialias(sec_img, antialias);

    update_hand();

    static lv_timer_t *clockTimer = lv_timer_create([](lv_timer_t *timer)
      {
        update_hand();
        update_peers();
        update_config();
      }, 1000, NULL
    );

}

// ---------------------------------------------------------------------------

lv_obj_t* append_peer(lv_obj_t *flex, char *nm, char *rssi, char *age)
{
  lv_obj_t *ln = lv_obj_create(flex);
  lv_obj_set_size(ln, 240, 35);
  lv_obj_set_style_pad_all(ln, 0, 0);
  lv_obj_add_style(ln, &peers_text_style, LV_PART_MAIN);

  lv_obj_t *label = lv_label_create(ln);
  lv_obj_set_width(label, 110);
  lv_label_set_text(label, nm);
  lv_obj_set_pos(label, 10, 5);

  label = lv_label_create(ln);
  lv_obj_set_width(label, 50);
  lv_label_set_text(label, rssi);
  lv_obj_set_pos(label, 130, 5);

  label = lv_label_create(ln);
  lv_obj_set_width(label, 40);
  lv_label_set_text(label, age);
  lv_obj_set_pos(label, 190, 5);

  return ln;
}

void update_peers(void)
{
    if (pageId != 2)
        return;

    while (lv_obj_get_child_cnt(tile_peers_list) > 4)
        lv_obj_del(lv_obj_get_child(tile_peers_list, 4));

    long now = millis();
    char buf1[10], buf2[10];
    for (int i = 0; i < MAX_HEARD_PEERS; i++) {
        struct peer_s *p = thePeers->heard_peers + i;
        if (p->id[0] == '\0')
            break;
        sprintf(buf1, "%d", int(p->rssi));
        sprintf(buf2, "%d", (now - p->when) / 1000);
        append_peer(tile_peers_list, p->id, buf1, buf2);
    }
}

void mk_tile_peers(lv_obj_t *tile)
{
    extern char ssid[];

    lv_style_init(&peers_title_style);
    // lv_style_set_text_decor(&peers_title_style, LV_TEXT_DECOR_UNDERLINE);
    lv_style_set_bg_color(&peers_title_style, lv_color_hex(0xc0c0c0));
    lv_style_init(&peers_text_style);
    lv_style_set_bg_color(&peers_text_style, lv_color_hex(0xe0e0e0));

    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t *label = lv_label_create(tile);
    lv_obj_add_style(label, &tile_title_style, LV_PART_MAIN);
    lv_label_set_text(label, "LoRa Peers");

    label = lv_label_create(tile);
    lv_obj_add_style(label, &tile_text_style, LV_PART_MAIN);
    lv_label_set_text(label, ssid);

    label = lv_label_create(tile);
    // lv_obj_add_style(label, &tile_text_style, LV_PART_MAIN);
    lv_label_set_text(label, "");

    lv_obj_t *title = append_peer(tile, "Peer", "RSSI", "Age");
    lv_obj_add_style(title, &peers_title_style, LV_PART_MAIN);
}

// ---------------------------------------------------------------------------

lv_obj_t* append_kv(lv_obj_t *flex, char *key, char *val)
{
  lv_obj_t *ln = lv_obj_create(flex);
  lv_obj_set_size(ln, 240, 30);
  lv_obj_set_style_pad_all(ln, 0, 0);
  lv_obj_add_style(ln, &tile_text_style, LV_PART_MAIN);
  lv_obj_set_style_border_width(ln, 0, LV_PART_MAIN);

  lv_obj_t *label = lv_label_create(ln);
  lv_obj_set_width(label, 105);
  lv_obj_set_pos(label, 5, 5);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_label_set_text(label, key);

  label = lv_label_create(ln);
  lv_obj_set_width(label, 115);
  lv_obj_set_pos(label, 120, 5);
  lv_obj_add_style(label, &config_text_style, LV_PART_MAIN);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_label_set_text(label, val);

  return ln;
}

void update_config(void)
{
    if (pageId != 3)
        return;

    while (lv_obj_get_child_cnt(tile_config_list) > 1)
        lv_obj_del(lv_obj_get_child(tile_config_list, 1));

    extern char *utc_compile_time;
    extern char ssid[];
    char buf[50];

    append_kv(tile_config_list, "this node", ssid);
    append_kv(tile_config_list, "compiled", utc_compile_time);

    append_kv(tile_config_list, "LoRa plan", the_lora_config->plan);
    int f = the_lora_config->fr / 10000;
    sprintf(buf, "%d.%02d MHz", f/100, f%100);
    append_kv(tile_config_list, "LoRa freq", buf);
    sprintf(buf, "%dkHz, %d",
            (int)(the_lora_config->bw/1000), the_lora_config->sf);
    append_kv(tile_config_list, "LoRa BW,SF", buf);

    sprintf(buf, "%d/%d (%d)", theGOset->goset_len,
            theGOset->largest_claim_span, theRepo->rplca_cnt);
    append_kv(tile_config_list, "# feeds", buf);
    sprintf(buf, "%d", theRepo->entry_cnt);
    append_kv(tile_config_list, "# entries", buf);
    sprintf(buf, "%d", theRepo->chunk_cnt);
    append_kv(tile_config_list, "# chunks", buf);

    int total = MyFS.totalBytes();
    int avail = total - MyFS.usedBytes();
    sprintf(buf, "%2d%% (%d MB)", avail / (total/100), avail/1024/1024);
    append_kv(tile_config_list, "avail. mem", buf);
}

void mk_tile_config(lv_obj_t *tile)
{
    lv_style_init(&config_text_style);
    lv_style_set_text_color(&config_text_style, lv_color_hex(0xD51B10));
    lv_style_set_bg_opa(&config_text_style, LV_OPA_COVER);
    // lv_style_set_bg_opa(&config_text_style, LV_OPA_TRANSP);

    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(tile, 8, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(tile);
    lv_obj_add_style(label, &tile_title_style, LV_PART_MAIN);
    lv_label_set_text(label, "Config");
    lv_obj_set_style_pad_bottom(label, 15, 0);
}

// ---------------------------------------------------------------------------

void mk_tile_about(lv_obj_t *tile)
{
    lv_obj_set_layout(tile, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t *label = lv_label_create(tile);
    lv_obj_add_style(label, &tile_title_style, LV_PART_MAIN);
    lv_label_set_text(label, "About tinySSB");
    // lv_obj_center(label);

    label = lv_label_create(tile);
    lv_obj_add_style(label, &tile_text_style, LV_PART_MAIN);
    lv_label_set_text(label,
                      "Feb 2025\n"
                      "https://github.com/\nssbc/tinySSB");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    logo_img = lv_img_create(tile);
    lv_img_set_src(logo_img, &tinySSB_logo_240x132);
    lv_img_set_zoom(logo_img, 210);
}

// ---------------------------------------------------------------------------

void tileview_change_cb(lv_event_t *e)
{
    lv_obj_t *tileview = lv_event_get_target(e);
    pageId = lv_obj_get_index(lv_tileview_get_tile_act(tileview));
    /*
    lv_event_code_t c = lv_event_get_code(e);
    Serial.print("# tile CB -- code=");
    Serial.print(c);
    uint32_t count =  lv_obj_get_child_cnt(tileview);
    Serial.print(" count=");
    Serial.print(count);
    */
    // Serial.print("# tile CB -- pageId=");
    // Serial.println(pageId);
}


UI_TWatch_Class::UI_TWatch_Class()
{
    Serial.println("# init of UI_TWatch_Class()");

    myid_valid = false;
    my_signing_fct = mk_signature;
    if (!NVS.begin("tinySSB"))
      Serial.println("#   NVS.begin() failed");
    else {
      Serial.println("#   NVS.begin() succeeded");
      unsigned char sk[64], pk[32];
      if (!ed25519_get_keypair("ed25519-seed", pk, sk)) {
        Serial.println("#     no seed found, generating one");
        if (ed25519_new_seed("ed25519-seed")) {
          Serial.println("#    new_seed OK");
          if (!ed25519_get_keypair("ed25519-seed", pk, sk)) {
            Serial.println("#    reading pk and sk worked now");
            myid_valid = true;
          } else
            Serial.println("#    reading pk and sk still failed");
        } else {
          Serial.println("#     new_seed failed, formatting NVS now");
          NVS.format();
        }
      } else {
        Serial.println("#     reading pk and sk worked");
        memcpy(myid_pk, pk, sizeof(pk));
        memcpy(myid_sk, sk, sizeof(sk));
        myid_valid = true;
      }
      if (myid_valid) {
        unsigned char buf[100];
        unsigned int len;
        mbedtls_base64_encode(buf, sizeof(buf), &len, myid_pk, sizeof(myid_pk));
        Serial.printf("#   public key is @%s.ed25519\r\n", buf);
        Serial.printf("#   public key is %s\r\n", to_hex(myid_pk, 32));
      }
    }

    lv_obj_t *label = 0;

    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_white());
    lv_style_set_text_color(&bg_style, lv_color_hex(0xff4040));

    scr = lv_scr_act();
    lv_obj_t *tmp;
    
    // background

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, &tremola);
    lv_obj_set_pos(img, 0, 0);

    logo_img = lv_img_create(scr); // transient logo
    lv_img_set_src(logo_img, &tinySSB_logo_240x132);
    lv_obj_center(logo_img);

    lv_task_handler();
    lv_obj_del(logo_img); // will be replaced by std background
 
    // spinner

    spin = lv_spinner_create(lv_scr_act(), 1500, 100);
    lv_obj_add_style(spin, &bg_style, LV_PART_ITEMS); // FIXME: no effect ...
    lv_obj_set_size(spin, 35, 35);
    lv_obj_set_pos(spin, 195, 10);
    lv_obj_add_flag(spin, LV_OBJ_FLAG_HIDDEN);

    // tiles

    lv_style_init(&bgTransparent);
    lv_style_set_bg_opa(&bgTransparent, LV_OPA_TRANSP);
    lv_style_init(&bg_grey_style);
    lv_style_set_pad_all(&bg_grey_style, 0);
    lv_style_set_bg_color(&bg_grey_style, lv_color_hex(0xe0e0e0));
    lv_style_set_border_width(&bg_grey_style, 0);
    lv_style_set_bg_opa(&bg_grey_style, LV_OPA_COVER);
    lv_style_init(&bg_green_style);
    lv_style_set_bg_color(&bg_green_style, lv_color_hex(0x02bb02));
    lv_style_init(&bg_red_style);
    lv_style_set_bg_color(&bg_red_style, lv_color_hex(0xff2020));

    lv_style_init(&tile_title_style);
    lv_style_set_text_color(&tile_title_style, lv_color_hex(0xD51B10));
    lv_style_set_text_font(&tile_title_style, &lv_font_montserrat_28);
    lv_style_set_bg_opa(&tile_title_style, LV_OPA_TRANSP);

    lv_style_init(&tile_text_style);
    lv_style_set_text_color(&tile_text_style, lv_color_black());
    lv_style_set_text_font(&tile_text_style, &lv_font_montserrat_18);
    lv_style_set_bg_opa(&tile_text_style, LV_OPA_TRANSP);

    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_add_style(tileview, &bgTransparent, LV_PART_MAIN);
    lv_obj_set_size(tileview, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // clock
    lv_obj_t *t0_0 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    tile_clock = t0_0;
    mk_tile_clock(t0_0);

    // chat
    lv_obj_t *t1_0 = lv_tileview_add_tile(tileview, 1, 0,
                                          LV_DIR_HOR | LV_DIR_BOTTOM);
    tile_chat = t1_0;
    mk_tile_chat(t1_0);

    // peers (name/RSSI/age)
    lv_obj_t *t2_0 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR);
    tile_peers_list = t2_0;
    mk_tile_peers(t2_0);

    // settings
    lv_obj_t *t3_0 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR);
    tile_config_list = t3_0;
    mk_tile_config(t3_0);

    // about
    lv_obj_t *t4_0 = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_HOR);
    mk_tile_about(t4_0);
}

// ---------------------------------------------------------------------------

void UI_TWatch_Class::add_new_post(char *txt, int t, int pos)
{
  Serial.printf("#   post %2d: \"%s\" %d\r\n", pos, txt, t);
  lv_obj_t *new_label;

  // FIXME: is strdup() necessary?
  if (post_cnt < 20) { // append
    new_label = lv_label_create(tile_chat_list);
    lv_obj_add_style(new_label, &post_text_style, LV_PART_MAIN);
    lv_obj_set_width(new_label, 240);
    lv_obj_set_style_text_align(new_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_pad_all(new_label, 5, 0);
    lv_label_set_long_mode(new_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(new_label, txt);
    post_cnt++;
    if (pos >= (post_cnt-1))
      return;
    // and now move to the right pos, see below
  } else { // overwrite first entry, before moving
    new_label = lv_obj_get_child(tile_chat_list, 1);
    lv_label_set_text(new_label, txt);
    pos--;
    if (pos <= 0)
      return;
  }
  lv_obj_move_to_index(new_label, pos + 1);
}

void UI_TWatch_Class::boot_ended()
{
  lv_obj_clear_flag(tile_chat, LV_OBJ_FLAG_HIDDEN); // after initializing
  lv_task_handler();
}

#endif // TINYSSB_BOARD_TWATCH

// eof
