/////////////////////////////////////////////////////////////////////////////////////////////
///
/// Platform module for Verix API
///
/// The purpose of this module is to provide access to Verix API's for things that differ
/// on other API's.  The goal is that a different platform module can be substituted
/// so the software can run on a different platform (such as ZK Software).
///
/// The main() function also goes in the platform-specific module.
///
///


#include <stdio.h> 
#include <stdlib.h>
#include <logsys.h>
#include <string.h>   
#include <svc.h>
#include <svctxo.h> 
#include <setjmp.h>
#include <stdarg.h>
#include "common.h"
#include "actutil.h"
#include "netsetup.h"
#include "vsocket.h"
#include "inet.h"
#include "hostdata.h"
#include "CoErrors.h"

//Include file for ACT2000

#include <acldev.h>
#include <aclstr.h>
#include <message.h>
#include <printer.h>  
#include <errno.h>

#include "lcdgraphics.h"
#include "platform.h"



// Definitions that apply only to Verix
char TranslateVerixKeyscanToASCII(char keyscan);


char VerixHWVersion[20];
char VerixEPROMVersion[20];
char VerixSerialNumber[20];

short hConsole = -1;

// Handle to serial printer
short hPrinter    = -1;    

// Handle to magnetic stripe reader device
short hMagReader    = -1;

// Handle to serial fingerprint reader, when open
short hSerialFingerPort = -1;


char *ThisAppVersion; 

char ApplicationMajorVersion[7] = "0.1.";
char ApplicationMinorVersion[4] = "001";
char TestVersionInfo[10] = ""; // this is where a string like BETA RC1 might go

int PlatformID=85; // This is just an arbitrarily chosen constant to identify the VeriFone Verix platform.  For ZK, it's 87.
char *PlatformName="Verix";

BOOL RunningOn510=FALSE;
BOOL RunningOn570=FALSE;
BOOL RunningOn610=FALSE;
BOOL TerminalHasGPRS=FALSE;
BOOL RunningOn610CDMA=FALSE;
BOOL RunningOn610WiFi=FALSE;


sem_t SleepSemaphore;
sem_t FileSemaphore;

		   

// Determines which letters can be accessed from which digits by the alpha key.
// Starting with a digit, the alpha key will advance to the next character to the right.
// If the next character to the right is a digit or *#, then it will search for and 
// backtrack to the digit or #* to the left.
// If this string does not START with a digit or *#, the alpha key routine can crash.
char *AlphaKeyMap = "0 ,-+1.QZqz2ABCabc@3DEFdef[4GHIghi]5JKLjkl=6MNOmno:7PRSprs8TUVtuv/9WXYwxy*},'\"{#&%()!";


void PokerGame(void);







BOOL IsDeveloperUnit(void) {

	// This is for adding *SMDL=1 and 7-to-reboot for developers.
	// (so a serial download can be initiated just by pressing 7).
	// This is a convenience to developers provided by VeriFone and documented in the SDK.
	// Feel free to add your development clock's serial number here.
	// When enabled, you can initiate a serial download from your PC (start the download on the PC first)
	// then press 7 to reboot, and the download will be immediately accepted without needing to access the system menu.

		// 209-339-471 = Vx570
		// 767-826-937 = Vx610 CDMA
		// 209-167-344

	if (strcmp(VerixSerialNumber,"209-339-471")==0) return TRUE;
	if (strcmp(VerixSerialNumber,"767-826-937")==0) return TRUE;
	if (strcmp(VerixSerialNumber,"209-167-344")==0) return TRUE;
	if (strcmp(VerixSerialNumber,"209-167-349")==0) return TRUE; // Mike Caldwell Vx570
	if (strcmp(VerixSerialNumber,"767-827-628")==0) return TRUE; // Vx610 GPRS/GSM
	if (strcmp(VerixSerialNumber,"766-063-667")==0) return TRUE; // Vx510 ether
	if (strcmp(VerixSerialNumber,"211-374-782")==0) return TRUE; // Vx510 GPRS
	return FALSE;
}






// Returns any of: 0123456789*# (obvious).  @ABCDE=Function keys F0-F5.  abcd=purple keys.
// ESC=cancel CR=enter BS=backspace $=alpha. ^N=Backspace held down
char TranslateVerixKeyscanToASCII(char keyscan) {
	// Number keys
	if (keyscan >= 0xB0 && keyscan <= 0xB9) return keyscan - 0x80;
	// F1 thru F4
	if (keyscan >= 0xFA && keyscan <= 0xFD) return keyscan - 0xFA + 'A';
	// purple keys
	if (keyscan >= 0xE1 && keyscan <= 0xE4) return keyscan - 0xE1 + 'a';
	switch (keyscan) {
		case 0x9B: return 0x1b;
		case 0x88: return 0x08;
		case 0x8E: return 0x0e;
		case 0x8f: return '$';
		case 0xEE: return '@'; // F0 present on Vx670 only
		case 0xEF: return 'E'; // F5 present on Vx670 only
		case 0x8D: return 0x0d;
		case 0xAA: return '*';
		case 0xA3: return '#';
	}
	return keyscan;
}





// This returns a fixed string describing the hardware.
char *Platform_GetHardwareInfo(void) {
	return VerixHWVersion;
}


// This returns a fixed string describing the hardware.
char *Platform_GetFirmwareInfo(void) {
	return VerixEPROMVersion;
}


char *Platform_GetHardwareSerialNumber(void) {
	return VerixSerialNumber;
}

void Platform_sleep_ms(unsigned int ms) {
	SVC_WAIT(ms);
}

void Platform_yield_ms(unsigned int ms) {
	// If there is other processing that should be called while the UI wants to sleep(e.g. network comms), this should be called here.
	SVC_WAIT(ms);

}


// Reboots hardware.  Does not return.
void Platform_Reboot(void) {
	SVC_RESTART("");
}



// Platform-specific code to render 
void Platform_RenderLCDMethod1(void) {
	// METHOD 1:
	// This works to render the whole screen, BUT the terminal must have any 16x16 font currently active.
	char renderbuf[128*64/8];
	int row4,col8,i16;
	char *idx1;
	char *idx2;
	char *d = renderbuf;
	// Screen is going to be drawn in cells of 16x16 pixels.
	// for each cell, need 16 bytes of first row, then 16 bytes of following row.
	// So we must rearrange the bytes into a new buffer, in the order they will be drawn.

	for (row4=0; row4<4; row4++) {
		// 128 = Screen Width
		// 16 = Height of each row in pixels
		// 128*16 = Number of pixels in an entire row
		// /8 = Compensates for 8 pixels in a byte.
		// 128*8 = Half a row of pixels - gets us to the bottom half of the row
		idx1 = &ScreenBuffer[row4*128*16/8];
		idx2 = &ScreenBuffer[row4*128*16/8+128*8/8];
		for (col8=0; col8<8; col8++) {
			for (i16=0; i16<16; i16++) *d++=*idx1++;
			for (i16=0; i16<16; i16++) *d++=*idx2++;
		}
	}
	
	gotoxy(0,0);		
	putpixelcol(renderbuf,ScreenWidthPixels*ScreenHeightByteRows);
	
}


void Platform_RenderLCDMethod2(void) {
	// METHOD 2:
	// This works to render the screen with a 6x8 font active, but the last two columns will always remain blank.
	// (VeriFone OS only lets us fill by character cells, and with a 6x8 font, only 126 columns can be reached)
	int row8;
	gotoxy(0,0);
	for (row8=0; row8<8; row8++) {
		putpixelcol(&ScreenBuffer[row8*128],126);
	}

}


void Platform_RenderLCD(void) {
	Platform_RenderLCDMethod1();
}


int Platform_fopen_readonly(char *filename) {
	return open(filename,O_RDONLY);
}

int Platform_fopen_append(char *filename) {
	return open(filename,O_APPEND+O_RDWR);
}

// Appends a file that may need to be aligned as an executable (Requirement on Verix for downloadable software updates)
int Platform_fopen_append_exe(char *filename) {
	return open(filename,O_APPEND+O_RDWR+O_CODEFILE);
}

int Platform_fopen_update(char *filename) {
	return open(filename,O_RDWR);
}

int Platform_fopen_create(char *filename) {
	return open(filename,O_CREAT+O_RDWR);
}

// Creates a file that may need to be aligned as an executable (Requirement on Verix for downloadable software updates)
int Platform_fopen_create_exe(char *filename) {
	return open(filename,O_CREAT+O_RDWR+O_CODEFILE);
}



int Platform_fclose(int handle) {
	
	return close(handle);
}

int Platform_fread(int handle, char *buf, int len) {
	return read(handle,buf,len);
}

// Nonblocking.
int Platform_read_pipe(int handle, char *buf, int len) {
	return read(handle,buf,len);
}

int Platform_fwrite(int handle, char *buf, int len) {
	return write(handle,buf,len);
}

int Platform_write_pipe(int handle, char *buf, int len) {
	return write(handle,buf,len);
}

int Platform_fseek(int handle, long offset, int origin) {
	return lseek(handle,offset,origin);
}

int Platform_delete_file(char *filename) {
	return _remove(filename);
}

int Platform_rename_file(char *oldfilename,char *newfilename) {
	return rename(oldfilename,newfilename);
}


// Puts the device in a suspended power state, if supported.
// Specific to Verix: Sleep only supported on battery-powered terminals while running on battery.
// Terminal will power off after being in sleep state long enough.
void Platform_powersleep(void) {

	set_backlight(0);

	read_evt(EVT_CONSOLE | EVT_KBD | EVT_SYSTEM); // Clear the events

	// Grab the sleep semaphore and hold onto it until sleep is done.
	// When comm thread tries to poll it, this will freeze the communications thread and prevent 
	// it from waiting on anything else that could wake the terminal during sleep.	
	// (If it is waiting on anything like a timer or Platform_sleep_ms the terminal will not stay asleep.)
	sem_wait(&SleepSemaphore);

	Platform_sleep_ms(1200); // Give the comm thread long enough to get stuck waiting on the semaphore,
	// which it polls every 1000ms while idle.

	SVC_SLEEP(); // The Sleep will not occur right now, but it will occur as soon as all threads are completely idle
	// (i.e. waiting on unavailable semaphore or untriggered events)
	
	wait_evt(EVT_CONSOLE | EVT_KBD | EVT_SYSTEM); // Wait for events signaling a wakeup or powerup, which shall be set upon awake due to *GKE=1 (set at startup)
	
	//LastUserActivityTick = read_ticks();

	set_backlight(1);

	// Release the semaphore, allowing the communications thread to start running.
	sem_post(&SleepSemaphore);
}



BOOL Platform_RunningOnBattery(void) {
	if (RunningOn610==FALSE) return FALSE;
	if (get_dock_sts()==0) return FALSE;
	return TRUE;
}


BOOL Platform_BatterySupported(void) {
	return (RunningOn610);
}


unsigned long Platform_read_ticks_ms(void) {
	return read_ticks();
}

int Platform_BatteryChargePercentage(void) {
	int BattFull,BattRemain;
	if (Platform_BatterySupported()==FALSE) return 0;
	BattFull = get_battery_value(FULLCHARGE);
	BattRemain = get_battery_value(REMAININGCHARGE);
	if (BattFull > 0 && BattRemain > 0) {
		BattRemain = BattRemain * 100 / BattFull;
		if (BattRemain < 0) BattRemain = 0;
		if (BattRemain > 100) BattRemain = 100;
		return BattRemain;
	}
	return 0;
}


// Returns a key from the device's keypad in ASCII.
// Returns 0 if timeout of timeoutseconds occurred.
// Timeoutseconds=0 means return buffered keypress (if any), but not to block waiting for one.
// Timeoutseconds=-1 means wait indefinitely for a keypress.
char Platform_GetKeyFromKeypad(int timeoutseconds) {	
	int ticks = timeoutseconds*20;
	char KeyPress=0;

	
	if (read(hConsole,(char*)&KeyPress,1)==1) {
		read_evt(EVT_KBD);
		return TranslateVerixKeyscanToASCII(KeyPress);
	}
	
	if (timeoutseconds==0) return 0;


	while (ticks-- > 0) {
		if (read(hConsole,(char*)&KeyPress,1)==1) {		
			read_evt(EVT_KBD);
			return TranslateVerixKeyscanToASCII(KeyPress);
		}
		Platform_yield_ms(50);
	}
	return 0;
}





int Platform_read_realtime_clock(char *yyyymmddhhmmssw) {
	// Unusual to VeriFone style, a return value of -1 indicates success here.
	if (read_clock(yyyymmddhhmmssw) == -1) return 1;
	return -1;

}

int Platform_set_realtime_clock(char *yyyymmddhhmmss) {
	int hClock = open(DEV_CLOCK, 0);
	if (hClock==-1) return -1;
	write(hClock, yyyymmddhhmmss, 14); // this wants 14 bytes: YYYYMMDDHHMMSS
	close(hClock);
	return 1;
}

// Opens the serial port most likely to have a serial finger reader attached on the specific device.
// If none, this should just return -1.
int Platform_open_finger_serial_port() {
open_block_t parm;
	int rv;
	// for Verix, 610 uses COM1, as does 510GPRS; everything else uses COM2.
	// So far I am assuming 38400 rather than enumerating all the possible rates... the finger reader in mind only uses 38400.
	if (RunningOn610 || (TerminalHasGPRS && RunningOn510)) {
		rv = open(DEV_COM1,0);
	} else {
		rv = open(DEV_COM2,0);
	}
	if (rv < 0) return rv;
	memset(&parm,0,sizeof(parm));
	parm.rate      = Rt_38400; // Rt_38400;
	parm.format    = Fmt_A8N1  |Fmt_RTS; 
	parm.protocol  = P_char_mode;
	parm.parameter = 0;
	set_opn_blk( rv, &parm);	
	return rv;
}

int Platform_open_printer_port(void) {
	return open(DEV_COM4,O_RDONLY);
}

void Platform_error_tone(void) {
	error_tone();
}

void Platform_normal_tone(void) {
	normal_tone();
}

// This is meant to poll the keyboard (and card reader, if present) for input, without actually
// swallowing the input, to determine whether one of these input methods is being used instead of finger reader.
// TRUE tells caller that keys have been pressed or magstripe has been swiped.
// Does not block.
BOOL Platform_peek_manual_input_event(void) {
	long eventmask = peek_event();						
	if ((eventmask & (EVT_KBD|EVT_MAG)) != 0) {
		return TRUE;
	}
	return FALSE;
}



// This shall poll the keyboard and card reader simultaneously.
// If a card is presented, this may overwrite the LCD line at CurrentYPosition to tell
// the user to try again if a misread occurred.
// 1 = Got ID by card reader
// 0 = cardreader not present, OR keypresses detected (available for subsequent keyboard reads)
// -1 = Cancelled due to too many retries
// -2 = Timeout

int Platform_Get_ID_by_card(char *buf, int timeoutseconds) {
	char magbuf[384];	
	int retries=6;
	int ticks = timeoutseconds * 20;
	long eventreceived;	



	// clear out events.  but if EVT_KBD was flipped, then there is probably key input.
	// This is important, because a call to this was probably preceded by a hit on the
	// fingerprint reader, which would have exited upon detecting a key event.
	
	eventreceived = read_evt(EVT_KBD);
	
	if (eventreceived) return 0;

	while (1) {
		int rv;
		int TempY = CurrentYPixelPosition;
		char *s;
		char *track1,*track2,*track3;


		while (1) {
			// this while loop will peek for events, but keep calling Platform_yield_ms while waiting.			
			eventreceived = peek_event();

			if (eventreceived & EVT_KBD) return 0;
			if (eventreceived & EVT_MAG) break;
			if (--ticks <= 0) return -2;
			Platform_yield_ms(50);
		}

		// Clear mag event
		read_evt(EVT_MAG);
		
		// point tracks at valid pointer
		track1=track2=track3="";

		// Attempt to read the mag
		rv = read(hMagReader,magbuf,384);

		// If swiped, then the card data is going to need parsing.
		// We are only interested in tracks that contain valid data, and 5, 12, or 14 digits.
		// note that key_card_entry adds 2 bytes to each track.
		if (rv > 10) {
			
			// Turn these into null-terminated strings where the 1st byte is the status byte.
			// Note, format is C1 S1 ... C2 S2 ... C3 S3 ...
			// where Cn is the length of all track n data (including the count byte)
			// and Sn is the status byte, and ... is any data read.
			track1=magbuf;
			track2=&magbuf[magbuf[0]];
			track3=&magbuf[magbuf[0]+magbuf[magbuf[0]]];
			track3[*track3]=0; // null for track3
			track3[0]=0; // null for track2
			track2[0]=0; // null for track1
			track1++,track2++,track3++; // now these all point at the status byte preceding the null-termed string.
			// THE STATUS BYTES NEED TO BE 0 TO INDICATE GOOD DATA.
			// IF DATA IS GOOD, ADJUST POINTER TO POINT AT GOOD DATA.  IF NOT, SET POINTER TO EMPTY STRING
			if (*track1==0) track1++; else track1="";
			if (*track2==0) track2++; else track2="";
			if (*track3==0) track3++; else track3="";

			// Find a track beginning with 9622 and length 12 or 14, in order of tracks 1,2,3
			if (strncmp(track1,"9622",4)==0 && (strlen(track1)==12 || strlen(track1)==14)) {
				strcpy(buf,track1+4);
				return 1;
			} else if (strncmp(track2,"9622",4)==0 && (strlen(track2)==12 || strlen(track2)==14)) {
				strcpy(buf,track2+4);
				return 1;
			} else if (strncmp(track3,"9622",4)==0 && (strlen(track3)==12 || strlen(track3)==14)) {
				strcpy(buf,track3+4);
				return 1;
			} else if (strlen(track1)==5) {
				strcpy(buf,track1);
				return 1;
			} else if (strlen(track2)==5) {
				strcpy(buf,track2);
				return 1;
			} else if (strlen(track3)==5) {
				strcpy(buf,track3);
				return 1;
			}
		}
		
		if (--retries==0) return -1;

		CurrentXPixelPosition=0;

		// flush mag
		while (read(hMagReader,magbuf,384) > 0);
					
		s="\v    \bPlease try again";
		// If tracks 1 and 2 were SOMETHING, then this is probably a Visa/Mastercard
		if (track1[0] != 0 && track2[0] != 0) {
			s="\v    Not a valid time card";
		}
		printLCD(s);				
		CurrentYPixelPosition = TempY;
		Platform_error_tone();
		Platform_yield_ms(800);
	
		

		
	}

}


// Verix doesn't currently support pre-emptive multitasking, but just in case it ever does...
// and this is required for Linux build anyway.
// This semaphore protects access to BATCH.DAT and TRANS.DAT.
void Platform_acquire_file_semaphore(void) {
	//Platform_debug_printf("Acquire File Semaphore");
	sem_wait(&FileSemaphore);
}


void Platform_release_file_semaphore(void) {
	//Platform_debug_printf("Release File Semaphore");
	sem_post(&FileSemaphore);
}

int Platform_set_startup_program(char *filename) {
	return put_env("*GO",filename,strlen(filename));
}


// Prints to debug console, if available, allowing printf-like formatting.  Buffer limit: 1024 chars
void Platform_debug_printf(char *fmt, ...) {
	char buf[1024];
	va_list argp;
	va_start(argp,fmt);
	vsprintf(buf,fmt,argp);
	va_end(argp);
	LOG_PRINTFF((0xFFFFFFFFL,"%s",buf));		
}

void Platform_flush_cardreader(void) {
	char magbuf[32];

	// read anything outstanding from the mag and keyboard to clear its buffer
	while (read(hMagReader,magbuf,32) > 0);
	read_evt(EVT_KBD|EVT_MAG);
}

int Platform_has_cardreader(void) {
	return 1;
}

// Call a feature function by name, if implemented.
// This is for including/excluding functions that are likely to be necessary/unnecessary based on hardware
// (for example, a menu that only exists for the SF600 reader won't be available on a device with no serial port)
// If the function doesn't exist, this returns -1.

int Platform_call_named_feature(char *funcname) {
	switch (funcname[0]) {
	case 'y':
		if (strcmp(funcname,"yield")==0) {
			yield();
			return 0;
		}
		break;
	case 'P':
		if (strcmp(funcname,"PokerGame")==0) {
			//PokerGame();
			return 0;
		}
		if (strcmp(funcname,"PollSleepSemaphore")==0) {
			// Grab and release the sleep semaphore.
			// If the UI thread is trying to put the terminal to sleep, these calls will hang until the sleep has ended.
			sem_wait(&SleepSemaphore);
			// Once the semaphore is acquired, immediately give it back up.
			sem_post(&SleepSemaphore);
		}
		break;

	case 'R':


		// ResetSMDL is accessed at the password menu, and applies to Verix only.
		// When set, it opens up the clock to be downloaded or controlled via COM1 immediately upon reboot.
		// This is basically a backdoor.  It can be used to reload programming
		if (strcmp(funcname,"ResetSMDL")==0) {
			ClearLCDScreen();
			printfLCD("SMDL has been set.\r\n"
			   "%s to reboot now.\r\n"
			   "%s to return to clock app.", LCDCancelKey, LCDEnterKey);
			put_env("*SMDL","1",1);
//			ShowPressEnterOrCancel();
//			if (GetEnterOrCancel() == FALSE) SVC_RESTART("");
			return 0;
		}
		break;
	case 'K':
		if (strcmp(funcname,"Keyclick")==0) {
			Platform_normal_tone();
			return 0;
		}
	}

	return -1;
}



BOOL Platform_has_modem(void) { return TRUE; }

BOOL Platform_has_Ethernet(void) {
	if (TerminalHasGPRS) return FALSE;				// for the 510
	if (RunningOn510 || RunningOn570) return TRUE;
	return FALSE;
}

BOOL Platform_has_GPRS(void) { return TerminalHasGPRS; }
BOOL Platform_has_CDMA(void) { return RunningOn610CDMA; }
BOOL Platform_has_WiFi(void) { return RunningOn610WiFi; }



// Support for Platform-Specific FingerPrint Reader (PSFPR)
// Verifone doesn't have one (this functionality is primarily for ZK reader)
BOOL Platform_PSFPR_Present(void) { return FALSE; }

// This should return the information that will be sent to the server.
// For ZKSoftware's private fingerprint library, this should be "ZK1".
// SF600 is not reported through this, since it is not a Platform Specific reader.
char *Platform_PSFPR_Version(void) { return ""; }
void Platform_PSFPR_Open(void) { return; }
void Platform_PSFPR_Close(void) { return; }
int Platform_PSFPR_VerifyAssociation(BOOL interactive)  { return -1; }
int Platform_PSFPR_Identify(void)  { return -1; }
int Platform_PSFPR_VerifyFID(int FID)  { return -1; }
int Platform_PSFPR_DeleteFID(int FID)  { return -1; }
int Platform_PSFPR_DeleteAll(void)  { return -1; }
int Platform_PSFPR_Init(void)  { return -1; }
int Platform_PSFPR_AddFingerHex(char *hexstring)  { return -1; }
char *Platform_PSFPR_Reassociate(void) { return NULL; }
char *Platform_PSFPR_GetBatchRecordCode(void) { return NULL; }

int Platform_PSFPR_VerifyTemporaryEnrollment(void) { return -1; }

int Platform_PSFPR_Enroll(BOOL IsFollowUp) { return -1; }
void Platform_PSFPR_LetGo(void) { return; } 
int Platform_PSFPR_CommitEnrollment(void) { return -1; }

void Platform_PSFPR_SaveNewEnrollmentToBatch(char *CardNumber) { return; }
void Platform_PSFPR_Sensitivity(void) { return; }
int Platform_Resolve_DNS(char *hostname, char *targetbuf) { return 0; }

void Platform_delete_data_files(void) { return; }

