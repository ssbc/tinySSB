#ifndef __ePaperSettings_GoodDisplay__
#define __ePaperSettings_GoodDisplay__

//
// Model GDEW026Z39 - 2.6 Inch 3-color ePaper display
//

const uint8_t deviceConfiguration_GDEW026Z39[] PROGMEM = 
{
	// booster soft start
	0,	0x06,
	3,	0x17,	0x17,	0x17,

	// power on command. Always "wait for ready" after this sequence
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x0F,

	//Resolution 152x296 pixels
	0,	0x61,
	3,	0x98,	0x01,	0x28,
		
	
	//Vcom and data interval setting
	0,	0x50,
	1,	0x77
};
const uint8_t deviceConfigurationSize_GDEW026Z39 PROGMEM = 23;

//
// Model GDEW027C44 - 2.7 Inch three color
//

const uint8_t deviceConfiguration_GDEW027C44[] PROGMEM = 
{
	// booster soft start
	0,	0x06,
	3,	0x07, 0x07, 0x17,
	
	// power optimizations
	0,	0xF8,
	2,	0x60, 0xA5,
	0,	0xF8,
	2,	0x73, 0x23,
	0,	0xF8,
	2,	0x7C, 0x00,

	// Reset DFV_EN
	0,	0x16,
	1,	0x00,
	
	// Power Setting SPI
	0,	0x01,
	5,	0x03, 0x00, 0x2B, 0x2B, 0x09,
	
	// power on command. Always "wait for ready" after this sequence
	0,	0x04,
	0xFF,		// wait for ready
	
	// Panel Setting
	0,	0x00,
	1,	0x0F,
	
	// PLL Control
	0,	0x30,
	1,	0x3A,
	
	// Resolution
	0,	0x61,
	4,	0x00, 0xB0, 0x01, 0x08,
	
	// VCM_DC Setting
	0,	0x82,
	1,	0x12,
	
	// VCOM and Data Interval
	0,	0x50,
	1,	0x87,
};

const uint8_t deviceConfigurationSize_GDEW027C44 PROGMEM = 59;

//
// Model GDEW029Z10 - 2.9 inch Three colors red e-paper display
//

const uint8_t deviceConfiguration_GDEW029Z10[] PROGMEM = 
{

	// booster soft start
	0,	0x06,
	3,	0x17,	0x17,	0x17,

	// power on command. Always "wait for ready" after this sequence
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	2,	0x0F, 0x0D,

	//Resolution 128x296
	0,	0x61,
	3,	0x80,	0x01,	0x28,		
	
	//Vcom and data interval setting
	0,	0x50,
	2,	0x77
};
const uint8_t deviceConfigurationSize_GDEW029Z10 PROGMEM = 24;

//
// Model GDEW0371Z80 - 3.71 inch Three colors red e-paper display
//

const uint8_t deviceConfiguration_GDEW0371Z80[] PROGMEM = 
{

	// booster soft start
	0,	0x06,
	3,	0x17,	0x17,	0x1D,

// 		panel power setting command
// 	0,	0x01,	
// 	5,	0x07,	0x07,	0x3F,	0x3F,	0x0D,

	// power on command. Always "wait for ready" after this sequence
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x0F,

	// 	PLL Control
// 	0,	0x30,
// 	1,	0x09,

	//Resolution 240x416
	0,	0x61,
	3,	0xf0,	0x01,	0xa0,

// 		VCOM_DC Setting
// 	0,	0x82,
// 	1,	0x12,
		
	
	//Vcom and data interval setting
	0,	0x50,
	2,	0x11, 0x07
};
const uint8_t deviceConfigurationSize_GDEW0371Z80 PROGMEM = 24;


//
// Model GDEW0371W7 - 3.71 inch 4 gray scale e-paper display
//

const uint8_t deviceConfiguration_GDEW0371W7[] PROGMEM = 
{
	// power settings
	0, 0x01,
	4, 0x07, 0x07, 0x3F, 0x3F,

	// booster soft start
	0,	0x06,
	3,	0x17,	0x17,	0x1D,

	// power on command. Always "wait for ready" after this sequence
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x3f,

	// PLL Setting
	0, 0x30,
	1, 0x04,
	
	//Resolution 240x416
	0,	0x61,
	3,	0xf0,	0x01,	0xa0,
		
	// vcom_dc setting
	0, 0x82,
	1, 0x08,
	
	//Vcom and data interval setting
	0,	0x50,
	2,	0x11, 0x07
};
const uint8_t deviceConfigurationSize_GDEW0371W7 PROGMEM = 39;

//
// Model GDEW042T2 - 4.2 inch 4 gray scale e-paper display
//

const uint8_t deviceConfiguration_GDEW042T2[] PROGMEM = 
{
	// Power Setting
	0,	0x01,
	5,	0x03, 0x00, 0x2B, 0x2B, 0x13,
	
	// Booster Soft Start
	0,	0x06,
	3,	0x17, 0x17, 0x17,
	
	// Power On
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x3F,
	
	// PLL Setting
	0,	0x30,
	1,	0x3C,
	
	// Resolution
	0,	0x61,
	4, 	0x01, 0x90, 0x01, 0x2C,
	
	// vcom_DC
	0,	0x82,
	1,	0x12,
	
	// VCOM and data interval
	0,	0x50,
	1,	0x92,
};
const uint8_t deviceConfigurationSize_GDEW042T2 PROGMEM = 40;

//
// Model GDEW0154T8 - 1.54 inch 4 gray scale e-paper display
//

const uint8_t deviceConfiguration_GDEW0154T8[] PROGMEM = 
{
	// Power Setting
	0,	0x01,
	5,	0x03, 0x00, 0x2B, 0x2B, 0x13,
	
	// Booster Soft Start
	0,	0x06,
	3,	0x17, 0x17, 0x17,
	
	// Power On
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x3F,
	
	// PLL Setting
	0,	0x30,
	1,	0x3C,
	
	// Resolution
	0,	0x61,
	3, 	0x98, 0x00, 0x98,
	
	// vcom_DC
	0,	0x82,
	1,	0x12,
	
	// VCOM and data interval
	0,	0x50,
	1,	0x97,
};
const uint8_t deviceConfigurationSize_GDEW0154T8 PROGMEM = 39;

//
// Model GDEW029T5 - 2.9 inch 4 gray scale e-paper display
//

const uint8_t deviceConfiguration_GDEW029T5[] PROGMEM = 
{
	// Power Setting
	0,	0x01,
	5,	0x03, 0x00, 0x2B, 0x2B, 0x13,
	
	// Booster Soft Start
	0,	0x06,
	3,	0x17, 0x17, 0x17,
	
	// Power On
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x3F,
	
	// PLL Setting
	0,	0x30,
	1,	0x3C,
	
	// Resolution
	0,	0x61,
	3, 	0x80, 0x01, 0x28,
	
	// vcom_DC
	0,	0x82,
	1,	0x12,
	
	// VCOM and data interval
	0,	0x50,
	1,	0x97,
};
const uint8_t deviceConfigurationSize_GDEW029T5 PROGMEM = 39;


//
// Model GDEW0213T5 - 2.13 inch 4 gray scale e-paper display
//

const uint8_t deviceConfiguration_GDEW0213T5[] PROGMEM = 
{
	// Power Setting
	0,	0x01,
	5,	0x03, 0x00, 0x2B, 0x2B, 0x13,
	
	// Booster Soft Start
	0,	0x06,
	3,	0x17, 0x17, 0x17,
	
	// Power On
	0,	0x04,
	0xFF,		// wait for ready

	// Panel Setting
	0,	0x00,
	1,	0x3F,
	
	// PLL Setting
	0,	0x30,
	1,	0x3C,
	
	// Resolution
	0,	0x61,
	3, 	0x68, 0x00, 0xD4,
	
	// vcom_DC
	0,	0x82,
	1,	0x12,
	
	// VCOM and data interval
	0,	0x50,
	1,	0x97,
};
const uint8_t deviceConfigurationSize_GDEW0213T5 PROGMEM = 39;


#endif //__ePaperSettings_GoodDisplay__
