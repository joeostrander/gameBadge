#include <avr/io.h>
#include "gameBadge.h"
#include <avr/pgmspace.h>

#define F_CPU 20000000UL

uint8_t buffer[SSD1306_LCDHEIGHT * SSD1306_LCDWIDTH / 8];

uint8_t scrollDirection = horizontal; //vertical_mirror; //

uint8_t winX[8];
uint8_t winXfine[8];
uint8_t winY = 0;
uint8_t winYfine = 0;

uint16_t tonePitch;
uint16_t toneTimer = 0;

uint8_t displaySleep = 0;

void displayInit(void) 
{

	csLow();	
	PORTA_OUTCLR = 0x10;				//LCD reset
	PORTA_OUTSET = 0x10;				//LCD reset
	csHigh();

    // Init sequence for 128x64 OLED module
    writecommand(SSD1306_DISPLAYOFF);                    // 0xAE
	
    writecommand(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    writecommand(0x80);									// the suggested ratio 0x80
    
    writecommand(SSD1306_SETMULTIPLEX);                  // 0xA8
    writecommand(0x3F);
    
    writecommand(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    writecommand(0x0);                                   // no offset
    
    writecommand(SSD1306_SETSTARTLINE);// | 0x0);        // line #0
    
    writecommand(SSD1306_CHARGEPUMP);                    // 0x8D
    writecommand(0x14);  // using internal VCC
    
    writecommand(SSD1306_MEMORYMODE);                    // 0x20
    //writecommand(0x01);									// 0x00 vertical addressing
    writecommand(0x00);									// 0x00 horizontal addressing
    
    writecommand(SSD1306_SEGREMAP | 0x01);				// normal
    writecommand(SSD1306_COMSCANDEC);					// normal
    
    writecommand(SSD1306_SETCOMPINS);                    // 0xDA
    writecommand(0x12);
    
    writecommand(SSD1306_SETCONTRAST);                   // 0x81
    writecommand(0x8F);
    
    writecommand(SSD1306_SETPRECHARGE);                  // 0xd9
    writecommand(0xF1);
    
    writecommand(SSD1306_SETVCOMDETECT);                 // 0xDB
    writecommand(0x40);
    
    writecommand(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    
    writecommand(SSD1306_NORMALDISPLAY);                 // 0xA6
    
    writecommand(SSD1306_DISPLAYON);                     //switch on OLED

}

uint8_t screenLoad(void) {						//This should be called beginning of each frame. Takes just under 1ms

	uint16_t pointer = 0;

	if (!displaySleep) {									//Don't bother with this if display is asleep (main code should do this, but this a sanity check)		
		csLow();
	
		for (int x = 0 ; x < 512 ; x++) {					//Send 512 pairs of 2 bytes over SPI in hardware buffer mode
			SPI0.DATA = buffer[pointer++];
			while(!(SPI0.INTFLAGS & SPI_DREIF_bm)) {}
			SPI0.DATA = buffer[pointer++];
			while(!(SPI0.INTFLAGS & SPI_DREIF_bm)) {}
		}
	
		csHigh();
	}

	uint8_t buttons = PORTC_IN & 0x0F;					//Grab lower nibble of C (d-pad)
	buttons |= PORTA_IN & 0xE0;							//OR in upper 3 bits of Port A
	return buttons;									//Return inverse for easy logic
		
}

void toneLogic(void) {			//Called at 1KHz

	if (toneTimer) {
		if (--toneTimer == 0) {
			TCA0.SINGLE.CTRLA &= 0xFE;				//Mask out enable bit (sound off)
		}
	}	
	
}

void tone(uint16_t thePitch, uint16_t theTime) {
	
    TCA0.SINGLE.CMP0 = thePitch;
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm; // Start	
	
	toneTimer = theTime;	
	
}

void ledState(uint8_t theState) {
	
	if (theState) {
		PORTB_OUTSET = 0x10;	
	}
	else {
		PORTB_OUTCLR = 0x10;
	}
	
}

void drawTiles(const char *tileData, uint8_t *tileMapPointer) {

	uint16_t bufPoint = 0;

	csLow();
	
	if (scrollDirection) {					//Vertical?
		
		uint8_t winYfineINV = 8 - winYfine;				//Get remainder of Y fine scrolling to OR in next line of pixels
		uint8_t coarseY = winY * 16;                           //Get the course Y value for top line of visible screen
		
		for (uint8_t row = 0 ; row < 8 ; row++) {               //Draw the 8 tile row high display

			uint8_t finePointer = winXfine[row];						//Copy the fine scrolling amount so we can use it as a byte pointer when scanning in graphics
			uint8_t coarseXpointerU = (tileMapPointer[coarseY] * 8) + finePointer;
			uint16_t coarseXpointerL; // = (tileMapPointer[coarseY + 16] * 8) + finePointer;
			if (coarseY == 240) {
				coarseXpointerL = (tileMapPointer[0] * 8) + finePointer;
			}
			else {
				coarseXpointerL = (tileMapPointer[coarseY + 16] * 8) + finePointer;
			}
					
			for (uint8_t colB = 0 ; colB < 128 ; colB++) {         //Draw 16 column wide display (8 pixels / 1 char at a time)

				uint8_t upper = pgm_read_byte(tileData + coarseXpointerU++) >> winYfine;
				uint8_t lower = pgm_read_byte(tileData + coarseXpointerL++) << winYfineINV;
				
				buffer[bufPoint++] = upper | lower;
	
				if (++finePointer == 8) {								//Done drawing this tile?
					finePointer = 0;                                //Reset fine scroll counter
					coarseXpointerU = tileMapPointer[coarseY] * 8;		//Get new tile pointer from memory
					if (coarseY == 240) {						
						coarseXpointerL = tileMapPointer[0] * 8;						
					}
					else {
						coarseXpointerL = tileMapPointer[coarseY + 16] * 8;						
					}	
				}				
			}

			// it's never vertical.. but somehow commenting this out helps???
			//TESTING!!!coarseY += 16;                                            //Increment coarse pointer to next row
		
		}		
	}
	else {								//Horizontal?
		
		uint8_t coarseY = 0;                           //Get the course Y value for top line of visible screen
  	
		for (uint8_t row = 0 ; row < 8 ; row++) {               //Draw the 8 tile row high display

			uint8_t finePointer = winXfine[row];						//Copy the fine scrolling amount so we can use it as a byte pointer when scanning in graphics
			uint8_t coarseX = winX[row];								//Find the current coarseX position for this line
			uint16_t coarseXpointer = (tileMapPointer[coarseX + coarseY] * 8) + finePointer;

			for (uint8_t colB = 0 ; colB < 128 ; colB++) {         //Draw 16 column wide display (8 pixels / 1 char at a time)

				buffer[bufPoint++] = pgm_read_byte(tileData + coarseXpointer++);
	
				if (++finePointer == 8) {									//Done drawing this tile?
					finePointer = 0;										//Reset fine scroll counter
					if (++coarseX > 31) {
						coarseX = 0;
					}
					coarseXpointer = tileMapPointer[coarseX + coarseY] * 8;		//Get new tile pointer from memory
				}
			
			}

			coarseY += 32;                                            //Increment coarse pointer to next row
		
		}		
	}

	csHigh();

}

void setWindow(uint8_t x, uint8_t y) {

	for (int row = 0 ; row < 8 ; row++) {
		winX[row] = x >> 3;
		winXfine[row] = x & 0x07;
		winY = y >> 3;
		winYfine = y & 0x07;		
	}
	
}

void setRowScroll(uint8_t x, uint8_t row) {

	winX[row] = x >> 3;
	winXfine[row] = x & 0x07;

}

void drawSprite(const char *bitmap, int8_t xPos, int8_t yPos, uint8_t frameNumber, int8_t mirror) {

	uint8_t xSize = pgm_read_byte(bitmap++);				//Get bitmap width from flash
	uint8_t ySize = pgm_read_byte(bitmap++) >> 3;			//Get bitmap height from flash, convert to bytes (8 pixels per byte)

	uint16_t sizeInBytes = xSize * ySize;					//Get total sprite size

	uint8_t offset = yPos & 0x07;
	uint8_t offsetInv = 8 - offset;
	
	int16_t bPointer = ((yPos >> 3) * 128) + xPos;			//Where the screen buffer pointer starts

	bitmap += frameNumber * (sizeInBytes * 2);				//Advance to frame pointer in sprite char memory

	if (mirror == -1) {										//If mirror draw sprites from right to left
		bitmap += xSize - 1;
	}
	
	uint8_t yOffset = 0;
	
	if (offset) {											//If sprite is on a byte boundry we can save a lot of time, so set flag if so
		yOffset = 1;
	}

	for (int y = 0 ; y < (ySize + yOffset) ; y++) {			//Do all rows of a sprite+1 if offset (since lowest row will go past a byte boundry) or if no offset, just do ySize rows

		int8_t xTemp = xPos;

		for (int16_t x = bPointer ; x < xSize + bPointer ; x++) {					//Select column

			if (!(x & 0xFC00) && !(xTemp & 0x80)) {									//Don't fill bytes outside of the screen buffer or past the left and right edges of the screen (xTemp)
				
				uint8_t buildMask = 0;
				uint8_t buildPixels = 0;
				
				if (offset) {														//Get offset pixels from adjacent rows
					if (y) {
						buildMask = pgm_read_byte(bitmap - xSize) >> offsetInv;
						buildPixels = pgm_read_byte((bitmap + sizeInBytes) - xSize) >> offsetInv;
					}
					if (y < ySize) {
						buildMask |= pgm_read_byte(bitmap) << offset;
						buildPixels |= pgm_read_byte(bitmap + sizeInBytes) << offset;
					}
				}
				else {																//Falls on byte boundry - simple and fast!
					buildMask = pgm_read_byte(bitmap);	
					buildPixels = pgm_read_byte(bitmap + sizeInBytes);				
				}
								
				buffer[x] &= ~buildMask;											//AND in mask
				buffer[x] |= buildPixels;											//OR in pixels
			}

			bitmap += mirror;															//Advance bitmap pointer and xTemp counter
			xTemp++;
		
		}
			
		bPointer += 128;														//Increment buffer line buffer by 1 byte-boundary line

		if (mirror == -1) {														//Since mirrored = going backwards in memory we have to skip 2 row widths to get to the right-hand start of the next one
			bitmap += (xSize * 2); // - 1;
		}

	}

}

void setScrollDirection(uint8_t mirrorType) {

	scrollDirection = mirrorType;	
	
}

void displayOnOff(uint8_t whatState) {
	
	SPI0_CTRLB = 0;											//Disable SPI buffer mode since we are sending single byte commands

	if (whatState) {
		displayInit();
		displaySleep = 0;									//Reset OLED to wake it back up
	}
	else {
		writecommand(SSD1306_DISPLAYOFF);                   //Sleep the OLED
		displaySleep = 1;
	}
	
	SPI0_CTRLB = SPI_BUFEN_bm;								//Resume SPI buffer mode
	
}

void writecommand(uint8_t c) {

	dcLow();
	csLow();
	sendSPI(c);
	csHigh();
	dcHigh();
		
}

void writedata(uint8_t c) {

	dcHigh();
	csLow();
	sendSPI(c);
	csHigh();

}

void sendSPI(uint8_t theData) {
	
	SPI0.DATA = theData;
	while(!(SPI0.INTFLAGS & SPI_IF_bm)) {}		//Wait for transfer
	
}

void dcHigh(void) {

	PORTB_OUTSET = 0x02;
	
}

void dcLow(void) {

	PORTB_OUTCLR = 0x02;
	
}

void csHigh(void) {

	PORTB_OUTSET = 0x01;
	
}

void csLow(void) {

	PORTB_OUTCLR = 0x01;
	
}

