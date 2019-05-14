// Main.c
// Runs on LM4F120/TM4C123
// Main program for the CAN example.  Initialize hardware and software for
// CAN transfers.  Repeatedly send the status of the built-in buttons over
// the CAN and light up the built-in LEDs according to the response.
// Daniel Valvano
// May 3, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
  Program 7.5, example 7.6

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// MCP2551 Pin1 TXD  ---- CAN0Tx PE5 (8) O TTL CAN module 0 transmit
// MCP2551 Pin2 Vss  ---- ground
// MCP2551 Pin3 VDD  ---- +5V with 0.1uF cap to ground
// MCP2551 Pin4 RXD  ---- CAN0Rx PE4 (8) I TTL CAN module 0 receive
// MCP2551 Pin5 VREF ---- open (it will be 2.5V)
// MCP2551 Pin6 CANL ---- to other CANL on network 
// MCP2551 Pin7 CANH ---- to other CANH on network 
// MCP2551 Pin8 RS   ---- ground, Slope-Control Input (maximum slew rate)
// 120 ohm across CANH, CANL on both ends of network
#include <stdint.h>
#include "PLL.h"
#include "Timer3.h"
#include "can0.h"
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "UART2.h"
#include "OS.h"
#include "Interpreter.h"
#include "Thread.h"
#include "fifo.h"
#include "user.h"


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode


unsigned long NumCreated =0;
int main(void){
	PLL_Init(Bus80MHz);              // bus clock at 80 MHz
	
	 /*-- ST7735 Init --*/
//    ST7735_InitR(INITR_REDTAB);
//    ST7735_FillScreen(ST7735_BLACK);
	
	 /*-- OS Init --*/
	OS_Init();
	
	 /*-- CAN Init --*/
	CAN0_Open();
	Timer3_Init(PeriodicSendCAN, 1600000); // initialize timer3 (10 Hz)


	PF1 = 0x00;		
	PF2 = 0x04;	
	PF3 = 0x00;
	
	//NumCreated += OS_AddThread(Thread_RcvCAN, 128, 2);
	NumCreated += OS_AddThread(&Interpreter, 128, 2);
	NumCreated += OS_AddThread(&IdleTask, 128, 7);
	
	OS_Launch(10000);
	
}

int amain()
{
	PLL_Init(Bus80MHz);              // bus clock at 80 MHz
	
	 /*-- OS Init --*/
	OS_Init();
	
	char word[10];
	word[0] = 'h';
	
	UART_OutString(word);
	char ch = UART_InChar();
	UART_OutChar(ch);
	return 0;
}


