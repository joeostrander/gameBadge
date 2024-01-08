#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include "drivers/gameBadge.h"
#include "game.h"
#include <string.h>

#define FRAME_FREQUENCY     (50)
#define FRAME_MS            (1000/FRAME_FREQUENCY)

uint8_t tileMap[256];							//Tilemap ram (2 screens of 16x8 tiles)
static volatile uint8_t sleepState = 0;			//0 = normal 1 = asleep
static volatile uint8_t frameFlag = 0;			//ms Frame counter to decide when a frame should be drawn

static volatile uint8_t secFlag = 0;			//This flag is set when a seconds transition occurs
static volatile uint8_t day = 0;				//0 = Sunday... 7 - Saturday
static volatile uint8_t amPM = 0;				//am = 0 pm = 1
static volatile uint8_t hours = 11;				//The hours
static volatile uint8_t minutes = 5;			//The minutes
static volatile uint8_t seconds = 0;			//The seconds

//Your flash game data here--------------------------------------------------------------------------------
const char hero[] PROGMEM = {

24, 24,
	
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xfc, 0xfe, 0xfe, 0xff, 0xff, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xfc, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf8, 0xe0, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xe1, 0xe3, 0xf3, 0xfb, 0xfd, 0xff, 0xff, 0xff, 0x1f, 0x0f, 0x1f, 0xff, 0xff, 0xff, 0xfd, 0xfb, 0xf3, 0xe3, 0xe1, 0xc0, 0x00, 0x00,
	
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0x3c, 0x9c, 0xdc, 0x5a, 0x46, 0x14, 0x50, 0xd0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xb0, 0x04, 0x1c, 0xfb, 0xf4, 0xe1, 0xeb, 0xea, 0xea, 0xe0, 0x12, 0x19, 0xa8, 0xf0, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x41, 0x61, 0x61, 0x78, 0x78, 0x73, 0x07, 0x07, 0x07, 0x07, 0x03, 0x73, 0x70, 0x78, 0x61, 0x61, 0x41, 0x40, 0x00, 0x00, 0x00, 		


};
const char beastTree[] PROGMEM = {
40, 40,
0x00, 0x80, 0xf8, 0xfc, 0xfc, 0xfc, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfe, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xe0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0x81, 0x00, 0x00, 0x00, 0xe0, 0xf8, 0xf0, 0xc0, 0x80, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x07, 0x1f, 0x1f, 0xff, 0xff, 0x1f, 0x1f, 0x0f, 0x07, 0xc1, 0xc0, 0xf0, 0xf1, 0xf1, 0xe7, 0x6f, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xfe, 0xff, 0xdf, 0x8f, 0xcf, 0xe3, 0xc1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x8c, 0xd8, 0xd8, 0xf0, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xc0, 0xc0, 0x80, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x10, 0xb8, 0x50, 0xa8, 0xa8, 0x54, 0x54, 0xaa, 0xaa, 0x44, 0x04, 0x0a, 0x8a, 0x14, 0x14, 0xaa, 0x88, 0x50, 0xfc, 0xa8, 0xa8, 0x54, 0x54, 0x14, 0x28, 0x88, 0x40, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0b, 0xf5, 0x55, 0xaa, 0xaa, 0x15, 0x45, 0x2a, 0xae, 0x5d, 0x5d, 0xba, 0xea, 0xd4, 0x50, 0xb0, 0xe2, 0xd5, 0x55, 0xbe, 0xe2, 0x55, 0x55, 0xd1, 0xaa, 0x2a, 0x10, 0x55, 0x2a, 0x0a, 0x7d, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0d, 0xf5, 0x2a, 0x6a, 0x40, 0x14, 0xaa, 0xa0, 0x45, 0x55, 0xaa, 0xaa, 0x55, 0xd7, 0x6e, 0x7a, 0xf7, 0xdd, 0x8a, 0x0f, 0x1d, 0x63, 0x81, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x30, 0xc0, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x07, 0x15, 0x0a, 0xfa, 0x11, 0x18, 0x0a, 0x06, 0x01, 0x80, 0x80, 0xa0, 0xa1, 0xe1, 0x46, 0x48, 0xf0, 0x00, 0x03, 0x04, 0x08, 0x24, 0xd2, 0x09, 0x0c, 0x82, 0xc1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x87, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x88, 0x88, 0x90, 0x90, 0x60, 0x30, 0x18, 0x07, 0x00, 0x8c, 0x72, 0x82, 0x8c, 0x71, 0x01, 0x3c, 0x63, 0x40, 0x80, 0x80, 0x80, 0x80, 
};

const char ship[] PROGMEM = {
	
24, 16,
0x04, 0x1e, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 0xf0, 0xf0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x10, 0x39, 0x3f, 0x3f, 0x3f, 0x1f, 0x1f, 0x1f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 
0x00, 0x04, 0x1c, 0xf4, 0xf4, 0xb4, 0xfc, 0x88, 0x88, 0x88, 0x98, 0x90, 0x90, 0x90, 0xa0, 0xe0, 0xc0, 0xc0, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x19, 0x1d, 0x1d, 0x0d, 0x0d, 0x0d, 0x05, 0x05, 0x05, 0x05, 0x05, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 
	
0x12, 0x7f, 0xff, 0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xf8, 0xf0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x40, 0xe2, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x1f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x01, 
0x00, 0x12, 0x72, 0xd6, 0xd6, 0xd4, 0xf4, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x60, 0x40, 0xc0, 0xc0, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x62, 0x7b, 0x7b, 0x3a, 0x3b, 0x3a, 0x1a, 0x1a, 0x1a, 0x1a, 0x1a, 0x02, 0x02, 0x03, 0x03, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x00, 

0x04, 0x0e, 0xfe, 0xfe, 0xfe, 0xfc, 0xfc, 0xfc, 0xf8, 0xf8, 0xf8, 0xf8, 0xf0, 0xf0, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x00, 0x20, 0x79, 0x7f, 0x7f, 0x7f, 0x3f, 0x3f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x01, 
0x00, 0x04, 0x0c, 0x3c, 0x7c, 0x78, 0x78, 0x78, 0x70, 0x70, 0x70, 0x70, 0x60, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x20, 0x31, 0x3b, 0x3b, 0x1b, 0x1b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x00, 	
		
};

const char debris[] PROGMEM = {
40,8,
0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfc, 0xfe, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 0xf7, 0xe7, 0xcf, 0x8f, 0x0f, 0x1f, 0x1f, 0x3f, 0x3f, 0x7e, 0x7e, 0xfc, 0xfc, 0xf8, 0xf8, 0xf0, 0xf0, 0xe0, 0xe0, 0xc0, 0xc0, 0x80, 0x80, 
0x00, 0x00, 0x80, 0x40, 0x20, 0x20, 0x50, 0x08, 0x04, 0x04, 0x8a, 0x41, 0x41, 0x20, 0x31, 0x28, 0x58, 0xb4, 0x62, 0xc3, 0x84, 0x04, 0x04, 0x08, 0x09, 0x11, 0x12, 0x22, 0x24, 0x44, 0x48, 0x88, 0x90, 0x10, 0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 
};

const char debrisTop[] PROGMEM = {
16,8,
0x00, 0x00, 0x80, 0xc0, 0xe0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xb8, 0x90, 
0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x20, 0x20, 0x10, 0x28, 0x04, 0x84, 0x44, 0x44, 0xa8, 0x90, 
};

const char tileData[] PROGMEM = {

0x00, 0x40, 0x40, 0x40, 0x58, 0x7c, 0x7c, 0x7e, 0x7e, 0x7a, 0x76, 0x7c, 0x78, 0x7a, 0x7c, 0x78, 0x7e, 0x7f, 0x7f, 0x7f, 0x7f, 0x7d, 0x63, 0x3e, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x40, 0x40, 0x40, 0x60, 0x60, 0x70, 0x74, 0x74, 0x7c, 0x7e, 0x7e, 0x7e, 0x7e, 0x7a, 0x74, 0x48, 0x30, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x30, 0x34, 0x30, 0x34, 0x34, 0x3e, 0x3e, 0x3e, 0x2a, 0x34, 0x18, 0x00, 0x10, 0x00, 0x10, 0x10, 0x18, 0x18, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x14, 0x1c, 0x0c, 0x08, 0x00, 0x10, 0x00, 0x10, 0x10, 0x18, 0x10, 0x10, 0x10, 0x10, 0x18, 0x18, 0x18, 0x14, 0x18, 0x10, 0x1f, 0xef, 0xf7, 0xfb, 0x3e, 0xdf, 0xff, 0xff, 0x7b, 0xb7, 0xcf, 0xfe, 0x7f, 0x8f, 0x77, 0xff, 0x3f, 0xdf, 0xef, 0xf7, 0xb7, 0xf6, 0x7b, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xd4, 0xd4, 0xe8, 0xe8, 0xf4, 0xf4, 0xea, 0xea, 0x80, 0xc0, 0xa0, 0x90, 0xa8, 0x88, 0x94, 0x83, 0x84, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x88, 0x80, 0x81, 0x84, 0x88, 0x90, 0xa2, 0xa1, 0x82, 0x8c, 0x90, 0x90, 0xa0, 0xa0, 0xc0, 0x80, 0xe0, 0x9c, 0xa3, 0x84, 0x80, 0x80, 0x81, 0x86, 0x80, 0x80, 0x80, 0x81, 0x80, 0x81, 0x83, 0x8f, 0x91, 0x81, 0x83, 0x8f, 0x97, 0x82, 0x82, 0x84, 0xa8, 0x90, 0xa0, 0xc0, 0xc0, 0xc0, 0x80, 0x80, 0x60, 0x90, 0x3e, 0x67, 0xfb, 0x7f, 0x3f, 0x3f, 0x7f, 0xff, 0xfe, 0x78, 0xf0, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xb8, 0xf0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x3e, 0x7f, 0xfb, 0xfe, 0x3e, 0x1c, 0x38, 0x60, 0x40, 0x80, 0x80, 0x80, 0xc0, 0xf0, 0xb8, 0xf0, 0xc0, 0x00, 0x00, 0x00, 0x01, 0x07, 0x17, 0x1f, 0x3f, 0x3f, 0x2f, 0x37, 0x1f, 0x1f, 0x0f, 0x0b, 0x0b, 0x0f, 0x0f, 0x07, 0x0f, 0x0f, 0x4f, 0x5f, 0x7f, 0x3f, 0x3f, 0x3f, 0x3f, 0x2f, 0x2f, 0x2f, 0x37, 0x1f, 0x07, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x4f, 0x5f, 0x5f, 0x5f, 0x5f, 0x7f, 0x3f, 0x3f, 0x2f, 0x2f, 0x37, 0x1b, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x17, 0x17, 0x1f, 0x1f, 0x0b, 0x0d, 0x07, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 

//0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xaa, 0x51, 0xaa, 0x55, 0xaa, 0x15, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0xaa, 0x55, 0xea, 0x55, 0xaa, 0x57, 0xaa, 0x55, 0x18, 0x84, 0xe4, 0x3f, 0x3f, 0xe4, 0x84, 0x18, 0x77, 0x07, 0x77, 0x70, 0x77, 0x07, 0x77, 0x70, 0x1c, 0x32, 0x7d, 0x7d, 0x6f, 0x3e, 0x1c, 0x00, 0x3a, 0x7d, 0x75, 0x45, 0x75, 0x7d, 0x3a, 0x00, 0x00, 0x18, 0x66, 0x81, 0x81, 0x66, 0x18, 0x00, 0x00, 0x10, 0x14, 0x68, 0x16, 0x28, 0x08, 0x00, 0x59, 0x82, 0x09, 0x20, 0x04, 0x91, 0x81, 0x6a, 0x08, 0x80, 0x01, 0x00, 0x10, 0x01, 0x80, 0x04, 0xf9, 0xff, 0x6f, 0x46, 0x6e, 0x7f, 0xff, 0xdd, 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18, 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81, 0x7c, 0xc0, 0x98, 0x2c, 0x34, 0x19, 0x03, 0x3e, 	

};

const uint8_t sineWave[] = {
32,34,35,37,38,40,41,43,
44,46,47,48,50,51,52,53,
55,56,57,58,59,59,60,61,
62,62,63,63,63,64,64,64,
64,64,64,64,63,63,63,62,
62,61,60,59,59,58,57,56,
55,53,52,51,50,48,47,46,
44,43,41,40,38,37,35,34,
32,30,29,27,26,24,23,21,
20,18,17,16,14,13,12,11,
9,8,7,6,5,5,4,3,
2,2,1,1,1,0,0,0,
0,0,0,0,1,1,1,2,
2,3,4,5,5,6,7,8,
9,11,12,13,14,16,17,18,
20,21,23,24,26,27,29,30,
};

//Your game variables (RAM) here-------------------------------------------------------------------------------
uint8_t heroFrame = 0;
uint8_t gamePad;

int16_t xPos = 0;
int16_t beastTreeX = 78; 
int8_t treeFlip = 1;
int16_t debrisX = 0;
int16_t yPos = 24;

uint8_t shipFrame = 0;
int8_t shipDir = 1;

int16_t floaty = 100;
uint8_t sinePointer[8] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8_t heroDir = 1;

uint16_t clouds = 0;

// uint16_t xx = 0;

int16_t baddie = 128;

uint8_t pos = 64;

void gameSetup(void) {							//This function is called once per boot/program

	setScrollDirection(horizontal);
	ledState(1);
	memset(tileMap, 47, 256);

	for (int x = 0 ; x < 4 ; x++) {			//Top clouds
		
		fillTiles(x * 8, 32, 8);
		
	}
	
	for (int x = 0 ; x < 4 ; x++) {			//Mid clouds
		
		fillTiles(32 + (x * 8), 0, 3);
		fillTiles(32 + (x * 8) + 4, 3, 3);		
		
	}	
	
	for (int x = 0 ; x < 2 ; x++) {			//Low clouds
		
		fillTiles(64 + (x * 16) + 2, 6, 2);
		fillTiles(64 + (x * 16) + 7, 8, 2);
		fillTiles(64 + (x * 16) + 11, 10, 2);		
		
	}
	
	for (int x = 0 ; x < 4 ; x++) {			//Mountains
		fillTiles((5*32) + (x * 8), 17, 8);	
		fillTiles((4*32) + (x * 8) + 1, 25, 2);
		fillTiles((4*32) + (x * 8) + 4, 29, 2);	
		fillTiles((3*32) + (x * 8) + 1, 27, 2);				
	}	

	for (int x = 0 ; x < 32 ; x++) {			//Ground BG	
		tileMap[(6*32) + x] = 16;		
	}
	
	for (int x = 0 ; x < 8 ; x++) {				//Ground FG
		tileMap[(7*32) + (x * 4) + 0] = 12;
		tileMap[(7*32) + (x * 4) + 1] = 13;
		tileMap[(7*32) + (x * 4) + 2] = 14;
		tileMap[(7*32) + (x * 4) + 3] = 15;					
	}

}

void systemLoop(void) {

    while (1) {
		if (sleepState) {
			sleep_cpu();
		}				
		else {
			if (frameFlag == FRAME_MS) {		//Time to draw a frame?
				frameFlag = 0;			//Reset ms counter
				ledState(1);			//LED is "on" for the frame

				gameFrame();			//New frame of game logic
				ledState(0);			//connect scope to this line to see frame time
			}	
		}
    }
	
}

void gameFrame(void) {							//This function is called at 50Hz

	gamePad = screenLoad();			//Draw the screen and get the buttons


	drawTiles(tileData, tileMap);
	
	drawSprite(beastTree, beastTreeX, 8, 0, treeFlip);				//Tree Y pos on byte boundry = MOAR SPEED
	
	drawSprite(ship, 20, yPos, shipFrame, shipDir);
	
	//if (floaty < 128) {
		//drawSprite(hero, floaty, (sineWave[sinePointer[0]++] >> 1), 0, 1);
		//if (sinePointer[0] == 128) {
			//sinePointer[0] = 0;
		//}
	//}
	//
	//if (--floaty < -40) {
		//floaty = 150;
		//sinePointer[0] = 0;
	//}
	
	drawSprite(debris, debrisX, 56, 0, normalSprite);					//Attaching fake parallax sprites on row boundries = faster sprite blit
	drawSprite(debrisTop, debrisX + 8, 48, 0, normalSprite);

	shipFrame = 0;			//Default

	if (gamePad & dB) {
		gotoSleep();
	}

	if (gamePad & dUp && yPos > 0) {
		yPos--;
		shipFrame = 2;
	}
	if (gamePad & dDown && yPos < 48) {
		yPos++;
		shipFrame = 1;
	}
	
	if (gamePad & dLeft) {
		shipDir = -1;
		xPos -= 1;
		// xx -= 3;
		floaty++;
		if (++beastTreeX == 128) {
			beastTreeX = -40;
		}
		clouds--;;
		debrisX += 4;
		if (debrisX > 127) {
			debrisX = -40;
		}
		
		tone(600, 200);
	}
	if (gamePad & dRight) {
		shipDir = 1;
		xPos += 1;
		clouds += 2;
		floaty--;
		if (--beastTreeX == -40) {
			beastTreeX = 128;
			if (treeFlip == 1) {
				treeFlip = -1;
			}
			else {
				treeFlip = 1;
			}
		}
		debrisX -= 4;
		
		if (debrisX < -40) {
			debrisX = 127;
		}

		tone(1200, 20);

	}
	
	if (gamePad & dMenu) {
		heroFrame = 4;
	}
	if (gamePad & dA) {
		heroFrame = 5;
	}
	if (gamePad & dB) {
		heroFrame = 6;
	}

	clouds--;

	setRowScroll(clouds >> 1, 0);				//Scroll rows 0 and 1 at a lower speed
	setRowScroll(clouds >> 2, 1);		//you
	setRowScroll(clouds >> 3, 2);		//you

	setRowScroll(xPos >> 1, 3);
	setRowScroll(xPos >> 1, 4);
	setRowScroll(xPos >> 1, 5);
	
	setRowScroll(xPos, 6);		//Scroll row 8 at a higher speed
	setRowScroll(xPos << 1, 7);			//Scroll row 8 at a higher speed
	
	ledState(0);
	
}

void gotoSleep(void) {

	sleepState = 1;
	displayOnOff(0);
	ledState(0);
	//Roll around to the main loop which will put us to sleep

}

void fillTiles(uint8_t location, uint8_t startingTile, uint8_t howMany) {
	
	for (int x = location; x < (int)(location + howMany); x++) {
		tileMap[x] = startingTile++;
	}
	
}

ISR(TCB0_INT_vect) {				//Timer trips? (This timer is disabled in Power-Down sleep mode)

	frameFlag++;					//Once 16 ms has passed, run frame (60 FPS... ish)
	toneLogic();					//Sound checks
	TCB0_INTFLAGS = 1;				//Clear flag
	
}

ISR(PORTA_PORT_vect) {				//PORTA ISR tied to Menu button for sleep wake-up

	if (sleepState) {				//Button change state with sleep mode enabled? WAKE UP! 
		sleepState = 0;
		displayOnOff(1);
	}
	
	PORTA.INTFLAGS = 0x20;			//Clear interrupt flag

}

ISR(RTC_PIT_vect) {						//ISR called every second

	if (++seconds == 60) {				//time lord stuff
		seconds = 0;
		if (++minutes == 60) {
			minutes = 0;
			++hours;
			if (hours == 12) {
				amPM++;					//amPM & 0x01 -> 0 = AM 1 = PM
			}
			if (hours == 13) {			//Time is so stupid. It's a bunch of rocks spinning around a star.
				hours = 1;
			}
		}
	}

	if (!sleepState) {					//If awake, change flag to indicate the seconds changed (for the display)
		secFlag = 1;
	}

	RTC.PITINTFLAGS = RTC_PI_bm;

}