#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_IMG_CONTACTS
#define LV_ATTRIBUTE_IMG_CONTACTS
#endif

const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_CONTACTS uint8_t contacts_map[] = {
  0x00, 0x00, 0x00, 0x00, 	/*Color of index 0*/
  0x00, 0x00, 0x00, 0x52, 	/*Color of index 1*/
  0x00, 0x00, 0x00, 0x94, 	/*Color of index 2*/
  0x00, 0x00, 0x00, 0xf4, 	/*Color of index 3*/

  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x06, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x90, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xbc, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x40, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x02, 0xff, 0xe0, 0x03, 0xfa, 0xf4, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x02, 0xff, 0xf0, 0x07, 0xd0, 0xb8, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x11, 0x00, 0x07, 0xc0, 0xbc, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x04, 0x00, 0x03, 0xd0, 0xb8, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x02, 0xff, 0xf4, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0xbf, 0xd0, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x15, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x2f, 0x00, 0x06, 0xff, 0xf4, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x1f, 0xff, 0xff, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x7f, 0x55, 0x6f, 0x80, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x04, 0x00, 0xbc, 0x00, 0x07, 0xd0, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x44, 0x40, 0xf4, 0x00, 0x03, 0xe0, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x02, 0xff, 0xf4, 0xf0, 0x00, 0x02, 0xe0, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x02, 0xff, 0xf1, 0xfa, 0xaa, 0xab, 0xe0, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xe0, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x04, 0x00, 0xaa, 0xaa, 0xaa, 0x90, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x1f, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xb8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x06, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0x90, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

/*
const lv_img_dsc_t contacts = {
  .header.cf = LV_IMG_CF_INDEXED_2BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 50,
  .header.h = 40,
  .data_size = 536,
  .data = contacts_map,
};
*/

const lv_img_dsc_t contacts = {
#if LV_BIG_ENDIAN_SYSTEM
  { 40, 50, 0, 0, LV_IMG_CF_INDEXED_2BIT, },
#else
  { LV_IMG_CF_INDEXED_2BIT, 0, 0, 50, 40, },
#endif
  536, contacts_map,
};

// eof