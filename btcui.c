#include <stdio.h> 
#include <stdlib.h>
#include <logsys.h>
#include <string.h>   
#include <svc.h>
#include <svctxo.h> 
#include <setjmp.h>
#include "common.h"

#include "platform.h"
#include "lcdgraphics.h"
#include "qrencode.h"

void Menu_NewAddress(void);
void Menu_Alerts(void);
void Menu_ScanPayment(void);
void Menu_Utils(void);

int main (int argc, char *argv[])
{
	char LastKeypress=0;
	int LastKeypressCount=0;
	
	// Get handles to system devices
	hConsole = open(DEV_CONSOLE,0);
	hMagReader = open(DEV_CARD,0);
	hPrinter = open(DEV_COM4,0);
	
	// Switch to a font where each character is 16x16 so the entire screen space can be utilized
	setfont("CARDS.VFT");
	InitLCDGraphics();
	
	
	// remainder of initialization stuff goes here.
	// read saved configuration file from filesystem.
	// put up the network, start talking to the server.
	
	while (1) {
		char c;
		char timebuf[20];
	
		EnableScreenBuffering(TRUE);
		ClearLCDScreen();	
		
		// put the menu options at absolute positions on the screen.
		// The \x15\x16 are special characters in the font that print an "F1" button in the space
		// of two characters.  The LCD functions interpret \b as "bold" (yes, the backspace character)
		// and \n as "normal" (yes, the newline character), and \r as a newline.
		CurrentYPixelPosition = 4;
		printLCDrj(0,"\nNew Address \x15\x16");
		CurrentYPixelPosition = 20;
		printLCDrj(0,"Alerts \x15\x17");
		CurrentYPixelPosition = 36;
		printLCDrj(0,"Scan Payment \x15\x18");
		CurrentYPixelPosition = 52;
		printLCDrj(0,"Utils \x15\x19");
		CurrentYPixelPosition = 0;
		
		// todo: poll the server to get the current exchange rate
		// dummy stuff goes here instead:
		printfLCD("\bBITCOIN\r\n\r\n\b%s\r\nBuy: %s\r\nSell: %s\r\r","MtGox","5.01","4.97");
		
		// Put the current time and date
		Platform_read_realtime_clock(timebuf);
		printfLCD("%c%c:%c%c:%c%c",timebuf[8],timebuf[9],timebuf[10],timebuf[11],timebuf[12],timebuf[13]);
		

		// Will draw the bitmap to the screen unless it is identical to what's on the screen now
		EnableScreenBuffering(FALSE);
		
		// poll the keypad
		c = Platform_GetKeyFromKeypad(0);
		if (c==0) {
			Platform_sleep_ms(100);
			continue;
		}
		
		// count repeat keypresses to trigger menus that must be reached by * * and # #
		if (LastKeypress==c) {
			LastKeypressCount++;
		} else {
			LastKeypressCount=1;
			LastKeypress=c;
		}
		
		switch (c) {
		case 'A': // F1
			Menu_NewAddress();
			break;
		case 'B': // F2			
			//Menu_Alerts();
			break;			
		case 'C': // F3
			//Menu_ScanPayment();
			break;
		case 'D': // F4
			//Menu_Utils();
			break;		
		case '*':
			if (LastKeypressCount==2) {
				//Menu_Admin();
			}
			break;
		case '#':
			if (LastKeypressCount==2) {
				//Menu_Dispenser();
			}		
			break;
		}
	}
}


void Menu_NewAddress(void) {

	int x,y;
	QRcode *code;
	unsigned char *curdot;
	int usedoublewidth=1;
	code = QRcode_encodeString("You asked for change, we gave you coins",0,QR_ECLEVEL_M,QR_MODE_8,1);
	
	if (code <= 0) {
		ClearLCDScreen();
		printfLCD("null, errno=%d", errno);
		Platform_GetKeyFromKeypad(120);
		return;
	}		

	curdot = code->data;
	if (code->width > 32) usedoublewidth=0;
	
	EnableScreenBuffering(TRUE);
	ClearLCDScreen();
	
	
	
	for (y=0; (y/2) < code->width && y<64; y++) {
		char *bufindex;
		unsigned char mask;
		curdot = &(code->data)[(y/2) * code->width];
		bufindex = &ScreenBuffer[(y/8) * 128 + (64 - code->width)]; 
		mask = 1 << (y & 7);
		for (x=0; x < code->width; x++) {
			if (*curdot & 1) {
				if (usedoublewidth) *bufindex++ |= mask;
				*bufindex++ |= mask;
			} else {
			    if (usedoublewidth) bufindex++;
				bufindex++;
			}
			curdot++;
		}
		if (usedoublewidth==0) y++;
	}
	
	
	EnableScreenBuffering(FALSE);

	// now send it to the printer.
	// put the printer in raster graphics mode.
	write(hPrinter, "\x1b" "g", 2);

	// send one line at a time.  each cell is 6x6 pixels.
	for (y=0; y < code->width; y++) {
		int r;
		char printbuf[60];
		for (r=0; r<6; r++) {
			curdot = &(code->data)[y * code->width];
			for (x=0; x < code->width; x++) {
				if (*curdot++ & 1) {
					printbuf[x]=0xff;
				} else {
					printbuf[x]=0xc0;
				}
			}
			printbuf[code->width] = 0x21; // line terminator
			write(hPrinter, printbuf, code->width+1);
			// todo: properly poll the printer status to determine how full the buffer is
			Platform_sleep_ms(10);			
		}
	}

	write(hPrinter, ")\n\n\n\n\n\n\n", 8);
	
	
	QRcode_free(code);
	Platform_GetKeyFromKeypad(120);
}

