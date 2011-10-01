// Common definitions
#ifndef __BOOL__
#define __BOOL__
typedef unsigned char   BOOL;           /* 0 or 1 */
#endif


#ifndef TRUE
#define TRUE        1
#endif
#ifndef FALSE
#define FALSE       0
#endif


extern short hConsole;

// Handle to serial printer
extern short hPrinter;    

// Handle to magnetic stripe reader device
extern short hMagReader;

// Handle to serial fingerprint reader, when open
extern short hSerialFingerPort;



// This identifies what platform is running, and is set by the specific platform module in main().
// 0 = Platform not set (error)
// 85 = VeriFone Verix (any)
// 87 = ZKSoftware (any)
extern int PlatformID;

// This identifies what platform is running, and is set by the specific platform module in main().
// 85 = "Verix"
// 87 = "ZK"
extern char *PlatformName;

extern char ApplicationMajorVersion[7];
extern char ApplicationMinorVersion[4];

// this is where a string like BETA RC1 might go.  If present, it gets displayed on screen.
extern char TestVersionInfo[10]; 



// This returns a fixed string describing the hardware.
// Example: VX570
char *Platform_GetHardwareInfo(void);
char *Platform_GetFirmwareInfo(void);
char *Platform_GetHardwareSerialNumber(void);

// Lists what letters are assigned to what number keys, for alphanumeric entry
extern char *AlphaKeyMap;

// Called by ClockIdle to get the platform-specific buttons drawn on the screen.
// Screen buffering will typically be turned on during this call.
void DrawButtonLegend(void);


// Thread sleep in milliseconds.  Consider using Platform_yield_ms instead, which will allow
// the thread to receive and respond to pipe messages during the delay.
void Platform_sleep_ms(unsigned int ms);

// Thread sleep in milliseconds, but polls and responds to pipe traffic where necessary.
void Platform_yield_ms(unsigned int ms);


// Reboots hardware.  Does not return.
void Platform_Reboot(void);

// Renders ScreenBuffer to LCD screen.
void Platform_RenderLCD(void);


// Acquire and release file access semaphore to ensure UI and comm threads aren't accessing files simultaneously.
// On platforms not supporting pre-emptive multitasking, these can just return without doing anything.
void Platform_acquire_file_semaphore(void);
void Platform_release_file_semaphore(void);

// platform-transparent fopen/fread/fwrite/fclose for files.
// hides differences in calls to open files, and deals strictly with int/char* types
int Platform_fopen_readonly(char *filename);
int Platform_fopen_append(char *filename);
int Platform_fopen_append_exe(char *filename);
int Platform_fopen_update(char *filename);
int Platform_fopen_create(char *filename);
int Platform_fopen_create_exe(char *filename);
int Platform_fclose(int handle);
int Platform_fread(int handle, char *buf, int len);
int Platform_read_pipe(int handle, char *buf, int len);
int Platform_fwrite(int handle, char *buf, int len);
int Platform_write_pipe(int handle, char *buf, int len);
int Platform_fseek(int handle, long offset, int origin);
int Platform_delete_file(char *filename);
// Delete platform-specific files as part of a factory reset.
void Platform_delete_data_files(void);
// Renames files.  This is used during the firmware upgrade process.
int Platform_rename_file(char *oldfilename,char *newfilename);

// Opens the serial port most likely to have a serial finger reader attached on the specific device,
// and sets its parameters to 38400,n,8,1
// If none, this should just return -1.
int Platform_open_finger_serial_port(void);

// Used by automatic updates - gets called upon command from server to change the
// startup program for next boot.  Only relevant to VeriFone platform.
int Platform_set_startup_program(char *filename);


int Platform_open_printer_port(void);

// Puts the device in a suspended power state, if supported.
// Specific to Verix: Sleep only supported on battery-powered terminals while running on battery,
// and only while no other threads are doing anything (Comm Thread needs to be waiting on
// clearance). Terminal will power off after being in sleep state long enough.
void Platform_powersleep(void);


// Read a count of ticks maintained by the operating system that counts milliseconds.
unsigned long Platform_read_ticks_ms(void);

// Tells whether the platform supports a dialup modem.
BOOL Platform_has_modem(void);

// Tells whether the platform supports TCP/IP over Ethernet.
BOOL Platform_has_Ethernet(void);

// Tells whether the platform supports TCP/IP over GPRS (GSM) cellular.
BOOL Platform_has_GPRS(void);

// Tells whether the platform supports TCP/IP over CDMA cellular.
BOOL Platform_has_CDMA(void);

// Tells whether the platform supports TCP/IP over WiFi.
BOOL Platform_has_WiFi(void);

// Tells whether the device is running on battery.  Always returns FALSE if device doesn't support battery.
BOOL Platform_RunningOnBattery(void);

// Returns TRUE if the hardware supports a battery (even if battery is not present or dead).
BOOL Platform_BatterySupported(void);

// Returns 0-100 to specify battery state of charge, 0 if battery not present or not supported.
int Platform_BatteryChargePercentage(void);

// Queries the platform for the current date and time.
int Platform_read_realtime_clock(char *yyyymmddhhmmssw);

// Asks the platform to set its realtime clock.
int Platform_set_realtime_clock(char *yyyymmddhhmmss);

char Platform_GetKeyFromKeypad(int timeoutseconds);

// This shall poll the keyboard and card reader simultaneously.
// 1 = Got ID by card reader into buf
// 0 = cardreader not present, OR keypresses detected (available for subsequent keyboard reads)
// -1 = Cancelled due to too many retries
// -2 = Timeout
int Platform_Get_ID_by_card(char *buf, int timeoutseconds);

// Tells whether the platform has a card reader (magstripe, proximity, Wiegand, etc.)
// This is used to determine whether card-related stuff should be displayed to the user.
// (card reader test screen, and "Swipe Card" prompts)
int Platform_has_cardreader(void);

// Flushes any received card swipes that have not been processed.
void Platform_flush_cardreader(void);

// Does a low-pitched speaker beep
void Platform_error_tone(void);

// Does a high-pitched speaker beep
void Platform_normal_tone(void);

// This polls the keyboard (and card reader, if present) for input, without actually
// swallowing the input, to determine whether one of these input methods is being used instead of finger reader.
// TRUE tells caller that keys have been pressed or card has been swiped.
// Does not block.
BOOL Platform_peek_manual_input_event(void);

// Prints to debug console, if available, allowing printf-like formatting.  Buffer limit: 1024 chars
void Platform_debug_printf(char *fmt, ...);


// Call a feature function by name, if implemented.
// This is for including/excluding functions that are likely to be necessary/unnecessary based on hardware
// (for example, a menu that only exists for the SF600 reader won't be available on a device with no serial port)
// If the function doesn't exist, this returns -1.
int Platform_call_named_feature(char *funcname);


// Support for Platform-Specific FingerPrint Reader (PSFPR)
// (i.e. not SF600, which is generic and serial)
// this should return TRUE if the device has a built-in fingerprint reader.
BOOL Platform_PSFPR_Present(void);

// This should return the information that will be sent to the server.
// For ZKSoftware's private fingerprint library, this should be "ZK1".
// SF600 is not reported through this, since it is not a Platform Specific reader.
char *Platform_PSFPR_Version(void);

// Opens access to platform-specific fingerprint reader
void Platform_PSFPR_Open(void);

// Closes access to platform-specific fingerprint reader, allows resources to be released.
void Platform_PSFPR_Close(void);

// Asks the fingerprint reader code to confirm that the fingerprint reader accessory has not
// been swapped out.  This only makes sense for removable fingerprint readers that store
// templates on-board - this function should always return nonzero if not the case.
// If interactive is true, then the function is allowed to interact with the user to tell
// them what is wrong and/or how to fix it.  If false, do not interact with user (this
// could be a transmit).
int Platform_PSFPR_VerifyAssociation(BOOL interactive);

// Directs the fingerprint reader to get a fingerprint and identify the user.
// This should wait until a timeout (typically 10sec) for the person to put their
// finger on the fingerprint reader.  Meanwhile it shall poll the card reader
// and keypad (without swallowing their input however), and shall signal if those
// devices indicate input before a fingerprint image is captured.
// You must have already Acquired, and to ensure accuracy, Verified Association.
// Return values:
// Positive number: the recognized FID (reader will have flashed green light)
// 0 = Keyboard or cardreader input detected first.
// -1 = Fingerprint reader error (not present, invalid response, etc.)
// -2 = User cancelled (reserved code: currently, pressing any key including CANCEL returns 0)
// -3 = A single fingerprint was captured but no matching FID was found.
//      (SF600 will have flashed red light)
int Platform_PSFPR_Identify(void);

int Platform_PSFPR_VerifyFID(int FID);

int Platform_PSFPR_DeleteFID(int FID);

int Platform_PSFPR_DeleteAll(void);

int Platform_PSFPR_Init(void);

int Platform_PSFPR_AddFingerHex(char *hexstring);

char *Platform_PSFPR_Reassociate(void);

// This returns the string that should be inserted into batch records
// sent to the SwipeClock server, to identify the type of reader.
// Codes defined so far: ZK1 (ZK proprietary).
// Similar code would be "SF6" (but not from PSFPR)
char *Platform_PSFPR_GetBatchRecordCode(void);

int Platform_PSFPR_VerifyTemporaryEnrollment(void);

int Platform_PSFPR_Enroll(BOOL IsFollowUp);

void Platform_PSFPR_LetGo(void);

int Platform_PSFPR_CommitEnrollment(void);

void Platform_PSFPR_SaveNewEnrollmentToBatch(char *CardNumber);

void Platform_PSFPR_Sensitivity(void);

// Platform-specific DNS resolver, if something other than gethostbyname() should be used.
// If resolution can occur, this returns 1 and puts IP address in targetbuf.
// Otherwise, returns 0.
int Platform_Resolve_DNS(char *hostname, char *targetbuf);


