// const-tbeam.h

#ifdef TINYSSB_BOARD_TBEAM

#if defined(HAS_LORA) && !defined(USE_RADIO_LIB)
# include <SPI.h>
# include <LoRa.h>
#endif

#ifdef TBEAM_07
# define GPS_TX 12
# define GPS_RX 15
#endif

# define SCK     5    // GPIO5  -- SX1278's SCK
# define MISO    19   // GPIO19 -- SX1278's MISO
# define MOSI    27   // GPIO27 -- SX1278's MOSI
# define SS      18   // GPIO18 -- SX1278's CS
# define RST     14   // GPIO14 -- SX1278's RESET
# define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

// user button
#ifdef TBEAM_07
# define BUTTON_PIN 39
#else
# define BUTTON_PIN 38  // this is for T_BEAM_V10
#endif

#endif

// eof
