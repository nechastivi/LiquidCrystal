// LiquidCrystal.c
// Runs on LM4F120/TM4C123 
// Library for Tiva C Series Lauchpad (TM4C123GH6PM) to control Liquid 
// Crystal Displays (LCDs) based on the Hitachi HD44780 (or a compatible) 
// chipset. 
// Repository 
// 		https://github.com/weynelucas/LiquidCrystal.git
// Lucas Weyne 
//		weynelucas@gmail.com
// 		https://github.com/weynelucas
// April 02, 2016

/*
	This library was built based on Arduino LiquidCrystal library, therefore, their behavior is similar.
	
	Hardware connections:
		* Using a 4-bit interface lenght, the least significant bits of microcontroller must be connected 
		to data pins DB4-DB7 of LCD
		* The R/W pin must be conntected to ground (this library only sends write commands)
		
	Specifications:
		* Using this library, you cannot use SysTick Timer in your project because is used by LiquidCrystal
		* All pins used on initialization must be configured as digital output before
	
	References:
		* Valvano, Jonathan W. Embedded Systems: Introduction to Arm Cortex-M Microcontrollers.  5th Edition, 2014.
		* Hitachi HD44780U datasheet - https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
		* Arduino LiquidCrystal Library - https://www.arduino.cc/en/Reference/LiquidCrystal
*/

typedef const enum{
	PA0 = 0x000, PA1, PA2, PA3, PA4, PA5, PA6, PA7,
	PB0 = 0x100, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
	PC0 = 0x200, PC1, PC2, PC3, PC4, PC5, PC6, PC7,
	PD0 = 0x300, PD1, PD2, PD3, PD4, PD5, PD6, PD7,
	PE0 = 0x400, PE1, PE2, PE3, PE4, PE5,
	PF0 = 0x500, PF1, PF2, PF3, PF4
}Pin;

typedef const enum{
	PortA, PortB, PortC, PortD, PortE, PortF
}Port;

typedef const enum{
	INTERFACE_8BITS, INTERFACE_4BITS
}DisplayInterface;

//********LiquidCrystal_Init*****************			
// Initializes the interface to the LCD screen
// Input: Pin that is connected to the register relect (RS) pin on the LCD
//        Pin that is connected to the enable (EN) pin on the LCD
//        Port where its least significant bits are connected to data pins (DB0-DB7) on the LCD
//        Interface (4 or 8 bits)
// Output: None
void LiquidCrystal_Init(Pin rs, Pin enable, Port data, DisplayInterface interface);

//********LiquidCrystal_SetCursor*****************
// Position the LCD cursor; that is, set the location at which subsequent text 
// written to the LCD will be displayed
// Input: Horizontal position on the LCD (0 to 15)
// 				Vertical position on the LCD (0 to 1)
// Output: None
void LiquidCrystal_SetCursor(unsigned char x, unsigned char y);

//********LiquidCrystal_Write*****************	
// Write a character to the LCD
// Input: Character to be written on the screen
// Output: None
void LiquidCrystal_Write(unsigned char chr);

//********LiquidCrystal_Print*****************	
// Prints text to the LCD
// Input: String to be written on the screen
// Output: None
void LiquidCrystal_Print(char *string);

//********LiquidCrystal_ScrollDisplayLeft*****************
// Scrolls the contents of the display (text and cursor) one space to the left
// Input: None
// Output: None
void LiquidCrystal_ScrollDisplayLeft(void);

//********LiquidCrystal_ScrollDisplayRight*****************	
// Scrolls the contents of the display (text and cursor) one space to the right
// Input: None
// Output: None
void LiquidCrystal_ScrollDisplayRight(void);

//********LiquidCrystal_Cursor*****************
// Display the LCD cursor: an underscore (line) at the position to which the  
// next character will be written
// Input: Cursor blink flag (0 - no blink, otherwise - blink)
// Output: None
void LiquidCrystal_Cursor(unsigned char blink);

//********LiquidCrystal_NoCursor*****************	
// Hides the LCD cursor
// Input: None
// Output: None
void LiquidCrystal_NoCursor(void);

//********LiquidCrystal_CreateChar*****************	
// Create a custom character (gylph) for use on the LCD
// Input: Character number (0 to 7)
//        Custom character data array (7 bytes)
// Output: None
void LiquidCrystal_CreateChar(unsigned char num, unsigned char *data);

//********LiquidCrystal_Clear*****************
// Clears the LCD screen and positions the cursor in the upper-left corner
// Input: None
// Output: None
void LiquidCrystal_Clear(void);

//********LiquidCrystal_Home*****************
// Positions the cursor in the upper-left of the LCD. That is, use that location 
// in outputting subsequent text to the display. To also clear the display, use 
// the LiquidCrystal_Clear() function instead
// Input: None
// Output: None
void LiquidCrystal_Home(void);


