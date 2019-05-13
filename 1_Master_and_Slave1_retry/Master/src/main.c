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
#include "Thread_sort.h"
#include "Interpreter.h"


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode
uint8_t XmtData1[8];
uint8_t XmtData2[8];
uint8_t RcvData[8];
uint32_t RcvCount=0;
uint8_t sequenceNum=0;  

void UserTask(void){
  XmtData1[0] = PF0<<1;  // 0 or 2
  XmtData1[1] = 0;  // 0
  XmtData1[2] = PF4>>1;  // 0 or 8
  XmtData1[3] = sequenceNum;  // sequence count
  CAN0_SendData1(XmtData1);
	
  XmtData2[0] = PF0<<1;  // 0 or 2
  XmtData2[1] = PF4>>2;  // 0 or 4
  XmtData2[2] = 0;       // unassigned field
  XmtData2[3] = sequenceNum;  // sequence count
  CAN0_SendData2(XmtData2);
	
  sequenceNum++;
}




unsigned long NumCreated;   // number of foreground threads created
int main()
{
	PLL_Init(Bus80MHz);              // bus clock at 80 MHz

	/*-- ST7735 Init --*/
//	ST7735_InitR(INITR_REDTAB);
//	ST7735_FillScreen(ST7735_BLACK);

	/*-- PortF_Init --*/
/*	SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
	while((SYSCTL_PRGPIO_R&0x20) == 0){};	
	GPIO_PORTF_LOCK_R = 0x4C4F434B;  // unlock GPIO Port F
	GPIO_PORTF_CR_R = 0xFF;          // allow changes to PF4-0
	GPIO_PORTF_DIR_R = 0x0E;         // make PF3-1 output (PF3-1 built-in LEDs)
	GPIO_PORTF_AFSEL_R = 0;          // disable alt funct
	GPIO_PORTF_DEN_R = 0x1F;         // enable digital I/O on PF4-0
	GPIO_PORTF_PUR_R = 0x11;         // enable pullup on inputs
	GPIO_PORTF_PCTL_R = 0x00000000;
	GPIO_PORTF_AMSEL_R = 0;          // disable analog functionality on PF
	*/
	/*-- CAN Init --*/	
	CAN0_Open();
		
	/*-- Timer3 Init --*/		
//	Timer3_Init(&UserTask, 1600000); // initialize timer3 (10 Hz)
	

	/*--OS Init --*/
	OS_Init();
	
	PF1 = 0x02;
	NumCreated = 0 ;
	NumCreated += OS_AddThread(Random_Input, 128, 2); // calls CAN_Send (priority 1)
	
	NumCreated += OS_AddThread(CAN_Receive, 128, 3);
//	NumCreated += OS_AddThread(Print_Result, 128, 4);

	NumCreated += OS_AddThread(IdleTask, 128, 7);
	
	OS_AddSW1Task(&Master_Reset,2);
	
	UART_OutCRLF();UART_OutCRLF();
	UART_OutString(" /************* START *************/"); UART_OutCRLF();

	OS_Launch(100000);
}
	
	





////////////////////////////////////////////////////////////////////////////


int canmain(void){
  PLL_Init(Bus80MHz);              // bus clock at 80 MHz
	
	    /*-- ST7735 Init --*/
//    ST7735_InitR(INITR_REDTAB);
//    ST7735_FillScreen(ST7735_BLACK);
		
	
  SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
                                   // allow time to finish activating
  while((SYSCTL_PRGPIO_R&0x20) == 0){};
  GPIO_PORTF_LOCK_R = 0x4C4F434B;  // unlock GPIO Port F
  GPIO_PORTF_CR_R = 0xFF;          // allow changes to PF4-0
  GPIO_PORTF_DIR_R = 0x0E;         // make PF3-1 output (PF3-1 built-in LEDs)
  GPIO_PORTF_AFSEL_R = 0;          // disable alt funct
  GPIO_PORTF_DEN_R = 0x1F;         // enable digital I/O on PF4-0
  GPIO_PORTF_PUR_R = 0x11;         // enable pullup on inputs
  GPIO_PORTF_PCTL_R = 0x00000000;
  GPIO_PORTF_AMSEL_R = 0;          // disable analog functionality on PF
  CAN0_Open();
  Timer3_Init(&UserTask, 1600000); // initialize timer3 (10 Hz)
  EnableInterrupts();
		
	PF1 = 0x02;
	PF2 = 0x00;	
	PF3 = 0x00;			

  while(1){
    if(CAN0_GetMailNonBlock(RcvData)){
      RcvCount++;
      PF1 ^= 0x02;
      PF2 = RcvData[1]; // blue
      PF3 = RcvData[2]; // green
			
//			ST7735_Message (1, 0, "King RCV_ID ", RCV_ID);
//			ST7735_Message (1, 1, "[0] ", RcvData[0]);
//			ST7735_Message (1, 2, "[1] ", RcvData[1]);
//			ST7735_Message (1, 3, "[2] ", RcvData[2]);
//			ST7735_Message (1, 4, "[3] ", RcvData[3]);
    }
  } 
}

