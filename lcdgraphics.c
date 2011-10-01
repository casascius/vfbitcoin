// lcdgraphics.c
//
// Generic graphic handler for 128x64 pixel monochrome display found in both Verix and ZKSoftware.
// Text and primitive graphics are handled here.
//
// This module is platform neutral, rendering its output to an in-memory bitmap.
// It relies on the Platform_render_LCD function to transfer that bitmap to the physical LCD screen.
//
// By Mike Caldwell, March 2009

#include "platform.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "lcdgraphics.h"



char ScreenBuffer[128*64/8];
int ScreenHeightPixels = 64;
int ScreenWidthPixels = 128;
int ScreenHeightByteRows = 8;

// Current drawing position (upper left corner)
int CurrentXPixelPosition=0;

// Current drawing position (upper left corner)
int CurrentYPixelPosition=0;


// The FONTS are handily edited by a utility in the VeriFone SDK that allows
// the creation of B&W font files for VeriFone terminals.  The file format is
// trivial and generic.  16-byte header, and then simple raw bitmaps for
// characters 0 thru 0xFF.
// VeriFone 8x16 font file is 2064 bytes long.  (we are reading two files in such a way that it results
// in 256 character entries in place of 128)
// I am using this exactly the same, except trailing FF's at the end of character definitions
// represent space that should not be rendered, in order to achieve proportional spacing.
// Font file has a 16-byte header, and then 128 16-byte entries.
// Each byte of the 16 bytes is a column of 8 pixels, LSB on top.
// First 8 bytes is top half (non-bold version), last 8 bytes is bottom half (bold version).
//unsigned char ProportionalFontData[4112];
extern unsigned char ProportionalFontData[4112];

//unsigned char BignumsData[2048];
extern unsigned char BignumsData[2048];

BOOL ScreenBuffering=FALSE;

// This keeps track of whether a banner is present in the top line of the screen.
// If so, calls to SetLCDTextRow() adjust to make up for its extra height.
BOOL BannerPresent=FALSE;

char *LCDBoldCommand = "\b";
char *LCDNormalCommand = "\n";
char *LCDTinyCommand = "\t";
char *LCDCancelKey = "\x10";
char *LCDEnterKey = "\xf";
char *LCDStarKey = "*";
char *LCDF1 = "\x15\x16";
char *LCDF2 = "\x15\x17";
char *LCDF3 = "\x15\x18";
char *LCDF4 = "\x15\x19";
char *LCDF5 = "\x15\x1a";
char *LCDF6 = "\x15\x1b";
char *LCDF7 = "\x15\x1c";
char *LCDF8 = "\x15\x1d";


int GetTextPixelWidthext(char *currentchar, BOOL usebold, BOOL usetiny, BOOL LimitToOneWord);


void InitLCDGraphics(void) {
//	int fontf=-1;

	memset(ScreenBuffer,0,sizeof(ScreenBuffer));

	// No longer need to read these files so long as the bytes are binhexed directly into the source.
	//fontf = Platform_fopen_readonly("PFONT2.VFT");
	//Platform_fread(fontf,(char*)&ProportionalFontData[2048],2064);
	//Platform_fclose(fontf);

	//fontf = Platform_fopen_readonly("PFONT.VFT");
	//Platform_fread(fontf,(char*)ProportionalFontData,2064);
	//Platform_fclose(fontf);

	//fontf = Platform_fopen_readonly("bignums.bmp");
	//Platform_fread(fontf,(char*)BignumsData,2048);
	//Platform_fclose(fontf);

	ClearLCDScreen();

}

// Enables or disables screen buffering.
// If you disable it, screen will instantly update.
void EnableScreenBuffering(BOOL Enable) {
	ScreenBuffering = Enable;
	if (ScreenBuffering==FALSE) Platform_RenderLCD();

}


// This clears the LCD screen in LCD buffer.
void ClearLCDScreen(void) {
	memset(ScreenBuffer,0,sizeof(ScreenBuffer));
	CurrentXPixelPosition=0;
	CurrentYPixelPosition=0;
	BannerPresent=FALSE;
	if (ScreenBuffering==FALSE) Platform_RenderLCD();
}

// Clears the LCD screen, leaving the banner intact (top 10 pixel rows) and the text
// cursor just below it.
void ClearLCDScreenLeavingBanner(void) {
	SetLCDTextRow(1);
	printLCD("\v\v");
}

// Clears the LCD screen, and puts a bold inverse banner.
void ClearLCDScreenWithBanner(char *banner) {
	BOOL tempScreenBuffering = ScreenBuffering;
	int ProposedFlags=LCD_NOWRAP + LCD_BANNER + LCD_BOLD;	
	
	// If bold text will push us off the edge of the screen, try non-bold.
	if (GetTextPixelWidthext(banner,TRUE,FALSE,FALSE) + 6 > ScreenWidthPixels) {
		ProposedFlags = LCD_NOWRAP + LCD_BANNER;
	}
	
	ScreenBuffering=TRUE;

	ClearLCDScreen();
	// darken the top of the screen, and put 1 space for a "left border"	
	printLCDext("\xb ",ProposedFlags,0,0);
	// write the text
	printLCDext(banner,ProposedFlags,0,0);
	// go to the next line.
	printLCDext("\r\n",ProposedFlags,0,0);

	ScreenBuffering = tempScreenBuffering;
	BannerPresent=TRUE;
	if (ScreenBuffering==FALSE) Platform_RenderLCD();

}


void SetLCDTextRow(int row0thru7) {
	CurrentXPixelPosition = 0;
	CurrentYPixelPosition = row0thru7 * 8;
	// if a banner is on the screen, rows 123456 start 2 pixels lower.
	if (BannerPresent && row0thru7 >= 1 && row0thru7 <= 6) CurrentYPixelPosition += 2;
}

// Sets X position, in pixels (e.g. 0-127)
void SetLCDPixelCol(int col) {
	CurrentXPixelPosition = col;
}


void printLCD(char *text) {
	printLCDext(text,0,0,0);
	// Called function already renders if screenbuffering is false.
}

// Prints right justified text with respect to RightInclusiveBound.
// Text cannot wrap, and goes no further left than CurrentXPixelPosition.
void printLCDrj(int RightInclusiveBoundOrZeroForScreenEdge, char *text) {
	int RightInclusiveBound = RightInclusiveBoundOrZeroForScreenEdge;
	
	int leftbound = CurrentXPixelPosition;
	int explen = GetTextPixelWidth(text);
	if (RightInclusiveBound==0 || RightInclusiveBound >= ScreenWidthPixels) RightInclusiveBound = ScreenWidthPixels-1;

	if (explen >= (RightInclusiveBound - CurrentXPixelPosition)) {
		// No adjustment necessary
	} else {
		leftbound = RightInclusiveBound - explen;
	}
	printLCDext(text,LCD_NOWRAP,leftbound,RightInclusiveBound);
	// put X back to beginning of line
	CurrentXPixelPosition = 0;

}

// Prints to LCD, allowing printf-like formatting.  Buffer limit: 1024 chars.
void printfLCDrj(int RightInclusiveBoundOrZeroForScreenEdge, char *fmt, ...) {
	char buf[1024];
	va_list argp;
	va_start(argp,fmt);
	vsprintf(buf,fmt,argp);
	va_end(argp);
	printLCDrj(RightInclusiveBoundOrZeroForScreenEdge, buf);
	// Called function already renders if screenbuffering is false.
}




// Prints to LCD, allowing printf-like formatting.  Buffer limit: 1024 chars.
void printfLCD(char *fmt, ...) {
	char buf[1024];
	va_list argp;
	va_start(argp,fmt);
	vsprintf(buf,fmt,argp);
	va_end(argp);
	printLCD(buf);
	// Called function already renders if screenbuffering is false.
}


void ShowPressString(char *what) {		
	int width,tempx,tempy;	
	width = GetTextPixelWidth(what);
	tempx = CurrentXPixelPosition;
	tempy = CurrentYPixelPosition;	
	CurrentXPixelPosition = ScreenWidthPixels - width;
	CurrentYPixelPosition = ScreenHeightPixels - 9;
	printLCDext(what,LCD_BANNER+LCD_NOWRAP,0,0);
	CurrentXPixelPosition = tempx;
	CurrentYPixelPosition = tempy;
	
	
}

// Puts a message at the bottom of the screen.
void ScreenBottomMessage(char *what) {
	int width,tempx,tempy,tempScreenBuffering;	
	width = GetTextPixelWidth(what);
	tempx = CurrentXPixelPosition;
	tempy = CurrentYPixelPosition;
	tempScreenBuffering=ScreenBuffering;
	ScreenBuffering=TRUE;
	CurrentXPixelPosition = 0;
	CurrentYPixelPosition = ScreenHeightPixels - 9;
	printLCDext("\xb",LCD_NOWRAP,0,0);
	printLCDext(what,LCD_NOWRAP,0,0);
	CurrentXPixelPosition = tempx;
	CurrentYPixelPosition = tempy;
	ScreenBuffering=tempScreenBuffering;
	if (ScreenBuffering==FALSE) Platform_RenderLCD();
}

void ShowPressEnter(void) {
	char pes[25];
	sprintf(pes," Press %s ",LCDEnterKey);
	ShowPressString(pes);
}

void ShowPressEnterOrCancel(void) {
	char pes[25];
	sprintf(pes," Press %s or %s ",LCDEnterKey,LCDCancelKey);
	ShowPressString(pes);
}

void ShowPressCancel(void) {
	char pes[25];
	sprintf(pes," Press %s ",LCDCancelKey);
	ShowPressString(pes);
}




// Writes some text to the LCD buffer.  Renders it too, if ScreenBuffering==FALSE.
// Text is the text to render.  Some control codes are accepted.  \x1e starts bold text.
// \x1f ends bold text.  \xc clears the screen.  \xb clears to end of line.
// TextStyle is 0 or any combination of LCD_BANNER LCD_BOLD LCD_NOWRAP.
// RightInclusiveBound says how far right on the screen the text is allowed to render.
// if zero it can render all the way to the edge of the screen.
void printLCDext(char *text, int TextStyle, int LeftInclusiveBound, int RightInclusiveBoundOrZeroForScreenEdge) {	
	unsigned char *currentchar;
	BOOL usebold=FALSE;
	BOOL usetiny=FALSE;
	int RightInclusiveBound = RightInclusiveBoundOrZeroForScreenEdge;
	if (CurrentXPixelPosition < LeftInclusiveBound) CurrentXPixelPosition = LeftInclusiveBound;

	

	if (TextStyle & LCD_BOLD) usebold=TRUE;

	// make sure we aren't planning on drawing too far right.	
	if (RightInclusiveBound < 16) RightInclusiveBound = ScreenWidthPixels-1;
	if (RightInclusiveBound >= ScreenWidthPixels) RightInclusiveBound = ScreenWidthPixels-1;

	for (currentchar=(unsigned char*)text; *currentchar; currentchar++) {
		int upperrow,lowerrow,charcol,shiftcount,charwidth,AddY;
		char *upperptr=NULL;
		char *lowerptr=NULL;

		int FontIndex = (*currentchar) * 16 + 16;
		if (usetiny) FontIndex += 128*16;
		if (usebold) FontIndex += 8;		
		// make sure character is valid
		if (FontIndex >= sizeof(ProportionalFontData)) FontIndex = 32 * 16 + 16;

		// Find out which row(s) we are going to render the text.
		// (our actual render is always up to 9 pixel rows due to descenders and/or banner)
		upperrow = CurrentYPixelPosition / 8;
		lowerrow = upperrow + 1;
		if (upperrow < ScreenHeightByteRows) upperptr = &ScreenBuffer[upperrow * ScreenWidthPixels + CurrentXPixelPosition];
		if (lowerrow < ScreenHeightByteRows) lowerptr = &ScreenBuffer[lowerrow * ScreenWidthPixels + CurrentXPixelPosition];
		shiftcount = CurrentYPixelPosition % 8;		
		charwidth=8; // maximum width of a character is 8 except where it ends in fe/ff
		while (ProportionalFontData[FontIndex+charwidth-1] == 0xff) charwidth--;

		switch (*currentchar) {
		case '\v':  // Clear to end-of-line (\v) or end of current line and every line there below (\v\v)

			if (TextStyle & LCD_BANNER) {			
				// Clear to end of line with black space, 9 pixels tall
				char topbyte,bottombyte;
				int themask = 0x1ff;				
				int xpix;
				if (shiftcount) themask <<= shiftcount;
				topbyte = themask & 0xff;
				bottombyte = (themask >> 8) & 0xff;
				for (xpix=CurrentXPixelPosition; xpix < ScreenWidthPixels; xpix++) {
					if (upperptr != NULL) *upperptr++ |= topbyte;
					if (lowerptr != NULL) *lowerptr++ |= bottombyte;
				}

			} else {
				// Clear to end of line with white space, 8 pixels tall
				char topbyte,bottombyte;
				int themask = 0xff;				
				int xpix;
				if (shiftcount) themask <<= shiftcount;				
				// Invert mask so we will be clearing the selected bits
				themask ^= 0xfffff;
				topbyte = themask & 0xff;
				bottombyte = (themask >> 8) & 0xff;

				for (xpix=CurrentXPixelPosition; xpix < ScreenWidthPixels; xpix++) {
					if (upperptr != NULL) *upperptr++ &= topbyte;
					if (lowerptr != NULL) *lowerptr &= bottombyte;					
					// if doing \v\v then clear all the way down
					if (currentchar[1] == '\v') {						
						// templowerrow is used as a count for how many times we should be going down one byte row
						int templowerrow = lowerrow;
						char *templowerptr;
						templowerptr = lowerptr;
						while (templowerrow < ScreenHeightByteRows && templowerptr != NULL) {
							// clear the current bottom byte
							*templowerptr=0;
							// move down one byte row
							templowerptr = &templowerptr[ScreenWidthPixels];
							templowerrow++;
						}
					}
					if (lowerptr != NULL) lowerptr++;
				}
			}
			continue;
		case '\t':
			usetiny=TRUE;
			continue;
		case '\n': // Line feed is treated as "normal" (tiny/bold off) since it's an "n".
			usebold=FALSE;
			usetiny=FALSE;
			continue;
		case '\r':  // Carriage return does the actual line feed as we..
			AddY=8;
			CurrentXPixelPosition=LeftInclusiveBound;
			if (TextStyle & LCD_BANNER) AddY=10;
			if (CurrentYPixelPosition + AddY < ScreenHeightPixels) {
				CurrentYPixelPosition += AddY;
			} else {
				goto returning;
			}
			continue;
		case '\xc':
			ClearLCDScreen();
			continue;
		case '\b':		
			usebold=TRUE;
			continue;
		case ' ': // Special case for space when wrapping is disabled
			if ((TextStyle & LCD_NOWRAP) == 0) {
				// Upon encountering a space, let's decide if the space and the next word will fit on the same line.
				// If it won't, we need to advance to the next line.
				// We only care if there IS a next line to begin with...
				if (CurrentYPixelPosition + 8 < ScreenHeightPixels) {
					int ExpectedNextWordLength = GetTextPixelWidthext((char*)currentchar,usebold,usetiny,TRUE);
					if (CurrentXPixelPosition + ExpectedNextWordLength > RightInclusiveBound) {
						// Next word will push us over.
						CurrentXPixelPosition=LeftInclusiveBound;
						CurrentYPixelPosition += 8;
						// Swallow any additional spaces.
						while (currentchar[1]==' ') currentchar++;
						continue;
					}
				}
			}
		}

		// Blit the columns of bitmap data directly into ScreenBuffer.
		for (charcol=0; charcol<charwidth; charcol++) {			
			int theword,themask;
			char topbyte,bottombyte;			
			theword = (int)((unsigned char)(ProportionalFontData[FontIndex+charcol]));			
			themask = 0xff;
			if (TextStyle & LCD_BANNER) {
				theword <<= 1;    // shift it down one pixel, and then
				theword ^= 0x1ff; // make 9 pixels of inverse if banner is enabled.
				themask = 0x1ff;  // make the clearing mask 9 pixels tall instead of 8
			}
			if (shiftcount > 0) {
				theword <<= shiftcount;
				themask <<= shiftcount;
			}

			// do the mask.  first, invert the mask because it represents bits we want to clear
			themask ^= 0xfffff;
			topbyte = themask & 0xff;
			bottombyte = (themask >> 8) & 0xff;
			if (upperptr != NULL) *upperptr &= topbyte;
			if (lowerptr != NULL) *lowerptr &= bottombyte;

			// now use bitwise OR to draw the text
			topbyte = theword & 0xff;
			bottombyte = (theword >> 8) & 0xff;

			if (upperptr != NULL) *upperptr++ |= topbyte;
			if (lowerptr != NULL) *lowerptr++ |= bottombyte;

			CurrentXPixelPosition++;
			// Increment X pixel position, and also move to the next line if we have wrapped the screen.
			if (CurrentXPixelPosition > RightInclusiveBound) {
				// If nowrap specified, stop rendering text.
				if (TextStyle & LCD_NOWRAP) goto returning;

				CurrentXPixelPosition=LeftInclusiveBound;
				if (CurrentYPixelPosition + 8 < ScreenHeightPixels) {
					CurrentYPixelPosition += 8;					
					upperrow++;
					lowerrow++;
				} else {
					goto returning;
				}
				upperptr=lowerptr=NULL;
				if (upperrow < ScreenHeightByteRows) upperptr = &ScreenBuffer[upperrow * ScreenWidthPixels + CurrentXPixelPosition];
				if (lowerrow < ScreenHeightByteRows) lowerptr = &ScreenBuffer[lowerrow * ScreenWidthPixels + CurrentXPixelPosition];
			}
		}
	}
returning:
	if (ScreenBuffering==FALSE) Platform_RenderLCD();
}

int GetTextPixelWidth(char *currentchar) {
	return GetTextPixelWidthext(currentchar,FALSE,FALSE,FALSE);
}

int GetTextPixelWidthext(char *currentchar, BOOL usebold, BOOL usetiny, BOOL LimitToOneWord) {
	int ExpectedNextWordLength=0;
	unsigned char *scanchar;
	for (scanchar=(unsigned char*)currentchar; *scanchar; scanchar++) {
		int thatcharwidth=8;
		int ScanFontIndex = (*scanchar) * 16 + 16;
		if (usetiny) ScanFontIndex += 128*16;
		if (usebold) ScanFontIndex += 8;		
		// make sure character is valid
		if (ScanFontIndex >= sizeof(ProportionalFontData)) ScanFontIndex = 32 * 16 + 16;
		
		
		

		// Any directives to go to the next line or to clear the screen will complete the count.
		if (*scanchar=='\r' || *scanchar=='\xc') break;
			

		// if encountered a space other than currentchar, that ends too.
		if (LimitToOneWord) {
			if (*scanchar==' ' && (scanchar != (unsigned char*)currentchar)) break;
		}

		if (*scanchar=='\n') {
			usebold=usetiny=FALSE;
			continue;
		}


		if (*scanchar=='\b') {
			usebold=TRUE;
			continue;
		}

		if (*scanchar=='\t') {
			usetiny=TRUE;
			continue;
		}

		// determine actual width of character.
		while (ProportionalFontData[ScanFontIndex+thatcharwidth-1]==0xff) thatcharwidth--;

		ExpectedNextWordLength += thatcharwidth;

	
	}
	return ExpectedNextWordLength;
}


// Draw a 1-pixel-thick horizontal line all the way across the screen.
void DrawLCDHorizontalRule(int PixelRow) {
	int i;
	char mask=1;	
	char *idx = &ScreenBuffer[PixelRow/8 * ScreenWidthPixels];
	if (PixelRow % 8) mask <<= (PixelRow % 8);
	for (i=0; i<ScreenWidthPixels; i++) *idx++ |= mask;
	if (ScreenBuffering==FALSE) Platform_RenderLCD();
}

static int NumberWidths[] = {27, 20, 27, 27, 27, 27, 27, 27, 27, 27, 12}; // widths of characters 0123456789:

void PutBigNums(char *nums) {
	int totalwidth=0;
	int col=0;
	char *n = nums;
	while (*n) {
		if (*n >= '0' && *n <= '9') {
			totalwidth += NumberWidths[(*n) - '0'];
		} else if (*n == ':' || *n==' ') {
			totalwidth += NumberWidths[10];
		}
		n++;
	}
	// determine starting column.
	col = 64 - (totalwidth/2);
	if (col < 0) col=0;

	while (*nums) {	
		if ((*nums >= '0' && *nums <= '9') || *nums==':') {
			// Draw a number at col.
			int y;
			int ThisNumberWidth = NumberWidths[(*nums) - '0'];
			if (*nums==':') ThisNumberWidth = NumberWidths[10];
			for (y=0; y<40; y++) {
				int i;
				int BufferRow = y/8;
				int BufferMask = 1 << (y&7);
				int bmpindex,bmpcol;
				char *ScreenBufferIndex = &ScreenBuffer[BufferRow * 128]; // The division is meant to drop the remainder before remultiplication
				bmpindex = 62 + (36 * (39-y));// bitmap header=62, each row is 36 bytes, starting from the bottom row
				bmpcol=0;
				for (i='0'; i<='9'; i++) if (*nums > i) bmpcol += NumberWidths[i-'0']; // determine starting column in bmpindex					
				for (i=0; i<ThisNumberWidth; i++) {
					int thispixelindex = bmpindex + ((bmpcol+i) / 8);
					int thispixelmask = 128 >> ((bmpcol+i) & 7);
					if ((BignumsData[thispixelindex] & thispixelmask) == 0) {
						if ((col+i) < 128) ScreenBufferIndex[col+i] |= BufferMask;
					}
				}

			}
			col += ThisNumberWidth;
		} else if (*nums==' ') {
			col += NumberWidths[10];
		}
		nums++;
		CurrentXPixelPosition=col; // for AM/PM if needed
		CurrentYPixelPosition=32;
	}
	if (ScreenBuffering==FALSE) Platform_RenderLCD();

}

// The first 62 bytes of a BMP file that contains a 128x64x2 image.
unsigned char bmpheader[62] = { 0x42, 0x4D, 0x3E, 4, 0, 0, 0, 0, 0, 0, 0x3E, 0, 0, 0, 0x28, 0,
                             0, 0, 0x80, 0, 0, 0, 0x40, 0, 0, 0, 1, 0, 1, 0, 0, 0,
							 0, 0, 0, 4, 0, 0, 0x13, 0x0B, 0, 0, 0x13, 0x0B, 0, 0, 2, 0,
							 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF };



// Writes the current screen to a bitmap file called "screen.bmp".
void ScreenshotBMP(void) {
	int i;
	unsigned char bmpdata[1024];
	unsigned char *outptr;
	int f = Platform_fopen_create("screen.bmp");	
	if (f<=0) return;

	// write the bitmap header.
	Platform_fwrite(f,(char*)bmpheader,sizeof(bmpheader));
	
	outptr = bmpdata; // point at output buffer
	memset(outptr,0,1024);

	// Bitmap data is Left to Right, BOTTOM to TOP, MSB leftmost.
	// Convert the LCD screen to BMP format, one pixel at a time.
	for (i=63; i>=0; i--) {
		int x;
		int BufferRow = i/8;
		int BufferMask = 1 << (i&7);
		char *ScreenBufferIndex = &ScreenBuffer[BufferRow * 128]; // The division is meant to drop the remainder before remultiplication
		for (x=0; x<128; x++) {
			// Note that White pixels (0 in our map) must be written in BMP as 1's, blacks vice versa.
			if ((ScreenBufferIndex[x] & BufferMask) == 0) {
				// Pixel is white.
				*outptr |= (128 >> (x&7));
			}
			if ((x&7)==7) outptr++;
		}
	}
	Platform_fwrite(f,(char*)bmpdata,1024);
	Platform_fclose(f);

}

