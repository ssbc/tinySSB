// const-t5gray.h

#ifdef TINYSSB_BOARD_T5GRAY

// epaper device is GDEW0213T5

#define SPI_SCK    18 // 14 // CLK
#define SPI_DIN    23 // 13

#define SRAM_CS -1

#define EPD_CS      5 // 15
#define EPD_DC     17 // 2
#define EPD_RESET  16 // 17
#define EPD_BUSY    4 // 16

#define PIN_KEY GPIO_NUM_39

#endif

// eof
