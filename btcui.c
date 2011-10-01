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


void Menu_NewAddress(void);
void Menu_Alerts(void);
void Menu_ScanPayment(void);
void Menu_Utils(void);

int main (int argc, char *argv[])
{
	int RedrawMenu=1;
	char LastKeypress=0;
	int LastKeypressCount=0;
	
	// Get handles to system devices
	hConsole = open(DEV_CONSOLE,0);
	hMagReader = open(DEV_CARD,0);
	
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
			//Menu_NewAddress();
			RedrawMenu=1;
			break;
		case 'B': // F2			
			//Menu_Alerts();
			RedrawMenu=1;		
			break;			
		case 'C': // F3
			//Menu_ScanPayment();
			RedrawMenu=1;
			break;
		case 'D': // F4
			//Menu_Utils();
			RedrawMenu=1;				
			break;		
		case '*':
			if (LastKeypressCount==2) {
				//Menu_Admin();
				RedrawMenu=1;
			}
			break;
		case '#':
			if (LastKeypressCount==2) {
				//Menu_Dispenser();
				RedrawMenu=1;
			}		
			break;
		}
	}
}

