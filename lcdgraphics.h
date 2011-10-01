// lcdgraphics.h: header file for lcdgraphics.c
//
// Generic graphic handler for 128x64 pixel monochrome display found in both Verix and ZKSoftware.
// Text and primitive graphics are handled here.
//
// This module is platform neutral, rendering its output to an in-memory bitmap.
// It relies on the Platform_render_LCD function to transfer that bitmap to the physical LCD screen.
//
// By Mike Caldwell, March 2009


// Initializes the LCD graphics.  Current implementation simply initializes
// the screen buffer to an empty display.
void InitLCDGraphics(void);

// Clears the LCD screen.  Cursor position is put at the upper left corner.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void ClearLCDScreen(void);

// Clears the LCD screen, then puts text at the top of the screen in inverse video.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
// Cursor position is in the upper left corner, immediately below the banner.
void ClearLCDScreenWithBanner(char *banner);

// Clears the LCD screen, but preserves a line of inverse text at the top of the screen.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
// Cursor position is in the upper left corner, immediately below the banner.
void ClearLCDScreenLeavingBanner(void);

// Writes to the LCD display, printf style.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void printfLCD(char *fmt, ...);

// Writes a static string to the LCD display.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void printLCD(char *text);

// Writes a static string to the LCD display, allowing for extended positioning options.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void printLCDext(char *text, int TextStyle, int LeftInclusiveBound, int RightInclusiveBoundOrZeroForScreenEdge);

// Prints right justified text with respect to RightInclusiveBound.
// Text cannot wrap, and goes no further left than CurrentXPixelPosition.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void printLCDrj(int RightInclusiveBoundOrZeroForScreenEdge, char *text);

// Formats and prints right justified text with respect to RightInclusiveBound.
// Text cannot wrap, and goes no further left than CurrentXPixelPosition.
// (If buffering is turned on, then the display is not immediately updated, only the buffer is affected)
void printfLCDrj(int RightInclusiveBoundOrZeroForScreenEdge, char *fmt, ...);


// Enables or disables screen buffering.
// When enabled, the display isn't redrawn until some sort of user input is expected.
// If you disable buffering, screen will instantly update with each command that draws to the display.
// ZKSoftware ZEM500 and later are visibly slow at updating the display,
// buffering multiple writes to the display makes a noticeable improvement.
void EnableScreenBuffering(BOOL Enable);

// Allows screen buffering status to be read externally.
// (Use EnableScreenBuffering instead to force render when needed)
extern BOOL ScreenBuffering;

// Screen buffer memory.
// Each byte is a column of 8 pixels, MSB on top, LSB on bottom.
// The first byte (ScreenBuffer[0]) is the upper left corner, each byte thereafter works its way left to right,
// and then row by row top to bottom.
extern char ScreenBuffer[];

// Height of screen in pixels.
extern int ScreenHeightPixels;

// Width of screen in pixels.
extern int ScreenWidthPixels;


extern int ScreenHeightByteRows;

// Current drawing position (relative to 0=upper left corner)
extern int CurrentXPixelPosition;

// Current drawing position (relative to 0=upper left corner)
extern int CurrentYPixelPosition;

// Positioning command - sets next text to draw on row of screen (0 thru 7 for 8-line display)
void SetLCDTextRow(int row0thru7);

// Positioning command - sets next text to draw starting at col
void SetLCDPixelCol(int col);

// Shows "Press Enter" at lower right corner of screen,
// substituting the correct keycap as necessary for the device.
void ShowPressEnter(void);
	
// Shows "Press Enter/Cancel" at lower right corner of screen,
// substituting the correct keycaps as necessary for the device.
void ShowPressEnterOrCancel(void);

// Shows "Press Cancel" at lower right corner of screen,
// substituting the correct keycap as necessary for the device.
void ShowPressCancel(void);

// Shows any other short string in the lower corner
void ShowPressString(char *what);

// Adds text to the last row (aka last 8 pixel rows) on the screen.  Used for realtime feedback
// during fingerprint reading.  Does not change Current[X/Y]PixelPosition.
void ScreenBottomMessage(char *what);

// These point to static strings you can use to print pre-defined graphics on screen or change fonts
// in the middle of text.
extern char *LCDBoldCommand;
extern char *LCDNormalCommand;
extern char *LCDTinyCommand;
extern char *LCDCancelKey;
extern char *LCDEnterKey;
extern char *LCDStarKey;
extern char *LCDF1;
extern char *LCDF2;
extern char *LCDF3;
extern char *LCDF4;
extern char *LCDF5;
extern char *LCDF6;
extern char *LCDF7;
extern char *LCDF8;



// Gets the number of horizontal pixels a given string would take to display on a single line,
// assuming there was no horizontal pixel limit.  (Used to estimate what will fit).
int GetTextPixelWidth(char *currentchar);

// Gets the number of horizontal pixels a given string would take to display.
// If usebold==TRUE, then evaluation starts out assuming the text begins bold, even without a control character making it bold.
// Same for usetiny==TRUE, assumption is text is tiny.
// If LimitToOneWord, then the string will be evaluated up to the next space instead of the whole string.
int GetTextPixelWidthext(char *currentchar, BOOL usebold, BOOL usetiny, BOOL LimitToOneWord);

void DrawLCDHorizontalRule(int PixelRow);

// Puts centered big numbers at the top of the screen.  (Only valid characters are "0123456789:")
// This displays the nice big SwipeClock time.
void PutBigNums(char *nums);

// Writes the current screen to a bitmap file called "screen.bmp".
void ScreenshotBMP(void);


// Definition for TextStyle parameter.  Signifies inverse video.  Text will be taller (10 pixels vs. 8)
// so it has a clean border that doesn't touch the characters below it.  CR/LF within a Heading
// will put writing point 10 pixels down.  Putting in a Ctrl-K (\xB) will extend inverse video to edge of screen.
// You should start your string with a space so the first character doesn't mask the left border.
#define LCD_BANNER 1

// Definition for TextStyle parameter.  Signifies use of bold text.  Bit flag may be added to other styles.
#define LCD_BOLD 2

// Definition for TextStyle parameter.  Signifies text may not wrap to next line(s). Bit flag may be added to other styles.
#define LCD_NOWRAP 4


