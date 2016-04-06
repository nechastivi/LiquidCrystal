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

#include "LiquidCrystal.h"

// ***********************************************************************
//	SysTick registers
// ***********************************************************************
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value

// ***********************************************************************
//	GPIO ports (base address)
// ***********************************************************************
#define GPIO_PORTA_BASE   0x40004000
#define GPIO_PORTB_BASE   0x40005000
#define GPIO_PORTC_BASE   0x40006000
#define GPIO_PORTD_BASE   0x40007000
#define GPIO_PORTE_BASE   0x40024000
#define GPIO_PORTF_BASE   0x40025000

// ***********************************************************************
//	GPIO pins offset (bit-specific addressing)
// ***********************************************************************
#define BIT0_OFFSET   0x0004
#define BIT1_OFFSET   0x0008
#define BIT2_OFFSET   0x0010
#define BIT3_OFFSET   0x0020
#define BIT4_OFFSET   0x0040
#define BIT5_OFFSET   0x0080
#define BIT6_OFFSET   0x0100
#define BIT7_OFFSET   0x0200

// ***********************************************************************
//	Commands map
// ***********************************************************************
#define DATA                    1
#define INSTRUCTION             0	
#define FUNCTION_SET_8BITS      0x30

// ***********************************************************************
//	Instructions map
// ***********************************************************************
#define CLEAR_DISPLAY_SCREEN              0x01
#define RETURN_HOME                       0x02
#define DISPLAY_OFF_CURSOR_OFF            DISPLAY_CTL
#define DISPLAY_ON_CURSOR_OFF             DISPLAY_CTL|DISPLAY_CTL_D	
#define DISPLAY_ON_CURSOR_ON              DISPLAY_CTL|DISPLAY_CTL_D|DISPLAY_CTL_C
#define DISPLAY_ON_CURSOR_BLINK           DISPLAY_CTL|DISPLAY_CTL_D|DISPLAY_CTL_C|DISPLAY_CTL_B
#define DISPLAY_CTL                       0x08
#define DISPLAY_CTL_D                     0x04        // Display on/off mask            (D=1: Display ON    D=0: Display OFF)
#define DISPLAY_CTL_C                     0x02        // Cursor on/of mask              (C=1: Cursor ON     C=0: Cursor OFF)
#define DISPLAY_CTL_B                     0x01        // Blink of cursor position mask  (B=1: Cursor blink  B=0: Cursor not blink)
#define SHIFT_DISPLAY_LEFT                0x18
#define SHIFT_DISPLAY_RIGHT               0x1C
#define INTERFACE_4BITS_2LINES_5X8_DOTS   FUNCTION_SET|FUNCTION_SET_N
#define INTERFACE_8BITS_2LINES_5X8_DOTS   FUNCTION_SET|FUNCTION_SET_DL|FUNCTION_SET_N
#define FUNCTION_SET                      0x20
#define FUNCTION_SET_DL                   0x10        // Data length mask     (DL=1: 8 bits     DL=0: 4 bits)
#define FUNCTION_SET_N                    0x08        // Number of lines mask (N=1 : 2 lines    N=0 : 1 line)
#define FUNCTION_SET_F                    0x04        // Character font mask  (F=1 : 5x10 dots  F=0 : 5x8 dots)	
#define CURSOR_MOVE_INC_DISPLAY_NO_SHIFT  ENTRY_MODE_SET|ENTRY_MODE_SET_ID
#define ENTRY_MODE_SET                    0x04				
#define ENTRY_MODE_SET_ID                 0x02        // Increments or Decrements mask  (ID=1: Increments     ID=0: Decrements)
#define ENTRY_MODE_SET_S                  0x01        // Accompanies display shift      (S=1 : Shift display  S=0 : No shift display)

// ***********************************************************************
//	Port and pin manipulation (auxiliar macros)
// ***********************************************************************
#define GetPinReg(pin)        ((volatile unsigned long *) (PortBaseAddress[(pin&0xF00)>>8] + PinAddressOffset[pin&0x0FF]))
#define GetPortDataReg(port)  ((volatile unsigned long *) (PortBaseAddress[port] + 0x03FC))
#define GetPinMask(pin)       (1<<(pin&0x0FF))
#define FunctionSet_8bits()   SendCommand(0x30, INSTRUCTION); EnablePulse();
#define FunctionSet_4bits()   SendCommand(0x20, INSTRUCTION); EnablePulse();

typedef struct {
	volatile unsigned long *address;
	unsigned char mask;
}LcdPin;

typedef struct{
	LcdPin rs;
	LcdPin enable;
	volatile unsigned long *data;
	unsigned char interface;
}Display;

const unsigned long PortBaseAddress[6] = {
	GPIO_PORTA_BASE,
	GPIO_PORTB_BASE,
	GPIO_PORTC_BASE,
	GPIO_PORTD_BASE,
	GPIO_PORTE_BASE,
	GPIO_PORTF_BASE
};

const unsigned long PinAddressOffset[8] = {
	BIT0_OFFSET,
	BIT1_OFFSET,
	BIT2_OFFSET,
	BIT3_OFFSET,
	BIT4_OFFSET,
	BIT5_OFFSET,
	BIT6_OFFSET,
	BIT7_OFFSET
};

const unsigned char DDRAM_Address[2][16] = {
	{0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F},
	{0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF}
};

Display Lcd;

// ***********************************************************************
//	Private functions prototypes
// ***********************************************************************
void SendCommand(unsigned char instructionOrData, unsigned char isData);
void TimerInit(void);
void TimerWait(unsigned long period);
void EnablePulse(void);
void Delayms(unsigned long time);
void Delayus(unsigned long time);

//********LiquidCrystal_Init*****************			
// Initializes the interface to the LCD screen
// Input: Pin that is connected to the register relect (RS) pin on the LCD
//        Pin that is connected to the enable (EN) pin on the LCD
//        Port where its least significant bits are connected to data pins (DB0-DB7) on the LCD
//        Interface (4 or 8 bits)
// Output: None
void LiquidCrystal_Init(Pin rs, Pin enable, Port data, DisplayInterface interface){
	TimerInit();
	
	Lcd.rs.address = GetPinReg(rs);
	Lcd.rs.mask = GetPinMask(rs);
	Lcd.enable.address = GetPinReg(enable);
	Lcd.enable.mask = GetPinMask(enable);
	Lcd.data = GetPortDataReg(data);
	Lcd.interface = interface;
	
	Delayms(20);   // wait for more than 15ms after Vcc rises to 4.5V
	
	*Lcd.rs.address = 0;   // Make RS pin low to send instructions
	
	FunctionSet_8bits();   // Function set (Interface is 8-bits long)
	Delayms(5);            // Wait for more than 4.1ms
	FunctionSet_8bits();   // Function set (Interface is 8-bits long)
	Delayus(200);          // Wait for more than 100us
	FunctionSet_8bits();   // Function set (Interface is 8-bits long)
	
	if(Lcd.interface == INTERFACE_8BITS){
		SendCommand(INTERFACE_8BITS_2LINES_5X8_DOTS, INSTRUCTION);  // 8-bit interface, 2 lines, 5x8 dots
	} else{
		FunctionSet_4bits();  // Function set (Set interface to be 4 bits long)
		SendCommand(INTERFACE_4BITS_2LINES_5X8_DOTS, INSTRUCTION);  // 4-bit interface, 2 lines, 5x8 dots 
	}
	
	SendCommand(DISPLAY_OFF_CURSOR_OFF, INSTRUCTION);             // Display off
	SendCommand(CLEAR_DISPLAY_SCREEN, INSTRUCTION);               // Display clear
	SendCommand(CURSOR_MOVE_INC_DISPLAY_NO_SHIFT, INSTRUCTION);   // Entry mode set (Increment by 1, no shift display)
	
	// Initialization ends
	SendCommand(DISPLAY_ON_CURSOR_OFF, INSTRUCTION);              // Display on, cursor off
	LiquidCrystal_SetCursor(0, 0);                                // Forcing cursor to beginning to 1st line
}

//********LiquidCrystal_SetCursor*****************
// Position the LCD cursor; that is, set the location at which subsequent text 
// written to the LCD will be displayed
// Input: Horizontal position on the LCD (0 to 15)
//        Vertical position on the LCD (0 to 1)
// Output: None
void LiquidCrystal_SetCursor(unsigned char x, unsigned char y){
	if(x<16 && y<2){
		SendCommand(DDRAM_Address[y][x], INSTRUCTION);
	}
}

//********LiquidCrystal_Write*****************	
// Write a character to the LCD
// Input: Character to be written on the screen
// Output: None
void LiquidCrystal_Write(unsigned char chr){
	SendCommand(chr, DATA);
}

//********LiquidCrystal_Print*****************	
// Prints text to the LCD
// Input: String to be written on the screen
// Output: None
void LiquidCrystal_Print(char *string){
	while(*string){
		SendCommand(*string, DATA);
		string++;
	}
}

//********LiquidCrystal_ScrollDisplayLeft*****************
// Scrolls the contents of the display (text and cursor) one space to the left
// Input: None
// Output: None
void LiquidCrystal_ScrollDisplayLeft(void){
	SendCommand(SHIFT_DISPLAY_LEFT, INSTRUCTION);
}

//********LiquidCrystal_ScrollDisplayRight*****************	
// Scrolls the contents of the display (text and cursor) one space to the right
// Input: None
// Output: None
void LiquidCrystal_ScrollDisplayRight(void){
	SendCommand(SHIFT_DISPLAY_RIGHT, INSTRUCTION);
}

//********LiquidCrystal_Cursor*****************
// Display the LCD cursor: an underscore (line) at the position to which the  
// next character will be written
// Input: Cursor blink flag (0 - no blink, otherwise - blink)
// Output: None
void LiquidCrystal_Cursor(unsigned char blink){
	SendCommand(blink ? DISPLAY_ON_CURSOR_BLINK : DISPLAY_ON_CURSOR_ON, INSTRUCTION);
}

//********LiquidCrystal_NoCursor*****************	
// Hides the LCD cursor
// Input: None
// Output: None
void LiquidCrystal_NoCursor(void){
	SendCommand(DISPLAY_ON_CURSOR_OFF, INSTRUCTION);
}

//********LiquidCrystal_CreateChar*****************	
// Create a custom character (gylph) for use on the LCD
// Input: Character number (0 to 7)
//        Custom character data array (7 bytes)
// Output: None
void LiquidCrystal_CreateChar(unsigned char num, unsigned char *data){unsigned char count;
	if(num < 8){
		SendCommand(0x40 + (num*8), INSTRUCTION);
		for(count=0; count<7; count++){
			SendCommand(data[count], DATA);
		}
		SendCommand(0x00, DATA);					// clear last line to filled by cursor
	}
}

//********LiquidCrystal_Clear*****************
// Clears the LCD screen and positions the cursor in the upper-left corner
// Input: None
// Output: None
void LiquidCrystal_Clear(void){
	SendCommand(CLEAR_DISPLAY_SCREEN, INSTRUCTION);
	LiquidCrystal_SetCursor(0, 0);
}

//********LiquidCrystal_Home*****************
// Positions the cursor in the upper-left of the LCD. That is, use that location 
// in outputting subsequent text to the display. To also clear the display, use 
// the LiquidCrystal_Clear() function instead
// Input: None
// Output: None
void LiquidCrystal_Home(void){
	LiquidCrystal_SetCursor(0, 0);
}

// ***********************************************************************
//	Private functions implementations
// ***********************************************************************

void EnablePulse(void){
	Delayus(1);
	*Lcd.enable.address |= Lcd.enable.mask;		// make enable pin high
	Delayus(1);
	*Lcd.enable.address = 0;									// make enable pin low
	Delayus(45);
}

void SendCommand(unsigned char instructionOrData, unsigned char isData){
	
	if(!isData){
		*Lcd.rs.address = 0;              // make RS pin low if command is instruction
	} else{
		*Lcd.rs.address |= Lcd.rs.mask;   // make RS pin high if command is data
	}
	
	if(Lcd.interface == INTERFACE_8BITS){
		*Lcd.data = instructionOrData;
	} else {
		*Lcd.data = (*Lcd.data&~0x0F)|((instructionOrData>>4)&0x0F);      // send higher nibble
		EnablePulse();
		*Lcd.data = (*Lcd.data&~0x0F)|(instructionOrData&0x0F);           // send lower nibble
	}
	
	EnablePulse();
	if(!isData && instructionOrData<4) Delayms(5);   // wait more time for clear display and return home instructions
}

void TimerInit(void){
	NVIC_ST_CTRL_R = 0;                   // disable SysTick during initialization
	NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M ; // maximum reload value
	NVIC_ST_CURRENT_R = 0;                // any value written to current clears it
	NVIC_ST_CTRL_R = NVIC_ST_CTRL_CLK_SRC + NVIC_ST_CTRL_ENABLE;  // enable SysTick with core clock
}

void TimerWait(unsigned long period){
	NVIC_ST_RELOAD_R = period - 1;
	NVIC_ST_CURRENT_R = 0;                         // any value written to current clears it
	while(!(NVIC_ST_CTRL_R&NVIC_ST_CTRL_COUNT));   // wait for count flag
}

void Delayms(unsigned long time){unsigned long count;
	for(count=0; count<time; count++){
		TimerWait(50000);   // wait 1ms
	}
}

void Delayus(unsigned long time){unsigned long count;
	for(count=0; count<time; count++){
		TimerWait(50);     // wait 1us
	}
}
