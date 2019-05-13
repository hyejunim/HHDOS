// can0.c
// Runs on LM4F120/TM4C123
// Use CAN0 to communicate on CAN bus PE4 and PE5
// 

// Jonathan Valvano
// May 2, 2015

/* This example accompanies the books
   Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers, Volume 3,  
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015

   Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers, Volume 2
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

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
#include "hw_can.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_types.h"
#include "can.h"
#include "debug.h"
#include "interrupt.h"

#include "can0.h"
#include "tm4c123gh6pm.h"
#include "fifo.h"

#define NULL 0
// reverse these IDs on the other microcontroller

#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))

// Mailbox linkage from background to foreground
uint8_t static RCVData1[8];
int static MailFlag1;
uint8_t static RCVData2[8];
int static MailFlag2;

typedef struct rcvdata
{
	uint8_t data[8];
} RCVDATA;

AddIndexFifo(USER1, 128, RCVDATA, 1, 0)	// uint8_t USER1Fifo[128]
AddIndexFifo(USER2, 128, RCVDATA, 1, 0)	// uint8_t USER2Fifo[128]



void Thread_RcvCAN(void)
{
	RCVDATA tmp;
	while(1){
		USER1Fifo_Get(&tmp);
		PF1 ^= 0x02;
		PF2 = tmp.data[1]; // blue
		PF3 = tmp.data[2]; // green
		
		USER2Fifo_Get(&tmp);
		PF1 ^= 0x02;
		PF2 = tmp.data[1]; // blue
		PF3 = tmp.data[2]; // green
	}
}

//*****************************************************************************
//
// The CAN controller interrupt handler.
//
//*****************************************************************************
void CAN0_Handler(void){ uint8_t data[8];
  uint32_t ulIntStatus, ulIDStatus;
  int i;
  tCANMsgObject xTempMsgObject;
  xTempMsgObject.pucMsgData = data;
  ulIntStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // cause?
  if(ulIntStatus & CAN_INT_INTID_STATUS){  // receive?
    ulIDStatus = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);
    for(i = 0; i < 32; i++){    //test every bit of the mask
      if( (0x1 << i) & ulIDStatus){  // if active, get data
        CANMessageGet(CAN0_BASE, (i+1), &xTempMsgObject, true);
        if(xTempMsgObject.ulMsgID == RCV_ID1){
			RCVDATA rcvdata;
			for(int i=0; i<xTempMsgObject.ulMsgLen; i++)
				rcvdata.data[i] = data[i];
			
			int sr = StartCritical();
			USER1Fifo_Put(rcvdata);
			EndCritical(sr);
			
		      MailFlag1 = true;   // new mail
        }
		if(xTempMsgObject.ulMsgID == RCV_ID2){

			RCVDATA rcvdata;
			for(int i=0; i<xTempMsgObject.ulMsgLen; i++)
				rcvdata.data[i] = data[i];
			
			int sr = StartCritical();
			USER2Fifo_Put(rcvdata);
			EndCritical(sr);
			
		      MailFlag2 = true;   // new mail
        }
      }
    }
  }
  CANIntClear(CAN0_BASE, ulIntStatus);  // acknowledge
}

//Set up a message object.  Can be a TX object or an RX object.
void static CAN0_Setup_Message_Object( uint32_t MessageID, \
                                uint32_t MessageFlags, \
                                uint32_t MessageLength, \
                                uint8_t * MessageData, \
                                uint32_t ObjectID, \
                                tMsgObjType eMsgType){
  tCANMsgObject xTempObject;
  xTempObject.ulMsgID = MessageID;          // 11 or 29 bit ID
  xTempObject.ulMsgLen = MessageLength;
  xTempObject.pucMsgData = MessageData;
  xTempObject.ulFlags = MessageFlags;
  CANMessageSet(CAN0_BASE, ObjectID, &xTempObject, eMsgType);
}
// Initialize CAN port
void CAN0_Open(void){uint32_t volatile delay; 

  MailFlag1 = false;
	MailFlag2 = false;
  SYSCTL_RCGCCAN_R |= 0x00000001;  // CAN0 enable bit 0
  SYSCTL_RCGCGPIO_R |= 0x00000010;  // RCGC2 portE bit 4
  for(delay=0; delay<10; delay++){};
  GPIO_PORTE_AFSEL_R |= 0x30; //PORTE AFSEL bits 5,4
// PORTE PCTL 88 into fields for pins 5,4
  GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFF00FFFF)|0x00880000;
  GPIO_PORTE_DEN_R |= 0x30;
  GPIO_PORTE_DIR_R |= 0x20;
      
  CANInit(CAN0_BASE);
  CANBitRateSet(CAN0_BASE, 80000000, CAN_BITRATE);
  CANEnable(CAN0_BASE);
// make sure to enable STATUS interrupts
  CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
// Set up filter to receive these IDs
// in this case there is just one type, but you could accept multiple ID types
  CAN0_Setup_Message_Object(RCV_ID1, MSG_OBJ_RX_INT_ENABLE, 8, NULL, RCV_ID1, MSG_OBJ_TYPE_RX);
  CAN0_Setup_Message_Object(RCV_ID2, MSG_OBJ_RX_INT_ENABLE, 8, NULL, RCV_ID2, MSG_OBJ_TYPE_RX);
  NVIC_EN1_R = (1 << (INT_CAN0 - 48)); //IntEnable(INT_CAN0);
  return;
}

// send 4 bytes of data to other microcontroller 
void CAN0_SendData1(uint8_t data[8]){
// in this case there is just one type, but you could accept multiple ID types
  CAN0_Setup_Message_Object(XMT_ID1, NULL, 8, data, XMT_ID1, MSG_OBJ_TYPE_TX);
}
// send 4 bytes of data to other microcontroller 
void CAN0_SendData2(uint8_t data[8]){
// in this case there is just one type, but you could accept multiple ID types
  CAN0_Setup_Message_Object(XMT_ID2, NULL, 8, data, XMT_ID2, MSG_OBJ_TYPE_TX);
}


// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail1(void){
  return MailFlag1;
}
int CAN0_CheckMail2(void){
  return MailFlag2;
}
// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock1(uint8_t data[8]){
  if(MailFlag1){
    data[0] = RCVData1[0];
    data[1] = RCVData1[1];
    data[2] = RCVData1[2];
    data[3] = RCVData1[3];
		data[4] = RCVData1[4];
		data[5] = RCVData1[5];
    data[6] = RCVData1[6];
    data[7] = RCVData1[7];
	  MailFlag1 = false;
    return true;
  }
  return false;
}

int CAN0_GetMailNonBlock2(uint8_t data[8]){
  if(MailFlag2){
    data[0] = RCVData2[0];
    data[1] = RCVData2[1];
    data[2] = RCVData2[2];
    data[3] = RCVData2[3];
		data[4] = RCVData2[4];
		data[5] = RCVData2[5];
    data[6] = RCVData2[6];
    data[7] = RCVData2[7];
	  MailFlag2 = false;
    return true;
  }
  return false;
}
// if receive data is ready, gets the data 
// if no receive data is ready, it waits until it is ready
/*
void CAN0_GetMail(uint8_t data[8]){
  while(MailFlag==false){};
  data[0] = RCVData[0];
  data[1] = RCVData[1];
  data[2] = RCVData[2];
  data[3] = RCVData[3];
	data[4] = RCVData[4];
  data[5] = RCVData[5];
  data[6] = RCVData[6];
  data[7] = RCVData[7];
  MailFlag = false;
}
*/
