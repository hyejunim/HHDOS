#include <stdint.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Thread_CAN.h"




//******** Thread_CANGetData  *************** 
// Get Data from CAN
// Kills itself after receiving
uint8_t CAN_rcvData[8];
void Thread_CANGetData(void){
	
	CAN0_GetMailNonBlock(CAN_rcvData);

	OS_Kill();
}

//******** Thread_CANSendData  *************** 
// Send Data to CAN
// Kills itself after sending
uint8_t CAN_sendData[8];
void Thread_CANSendData(void){
	// example of master sending the data
	// CAN0_SendData1(CAN_sendData);
	// CAN0_SendData2(CAN_sendData);
	
	// example of slave sending the data
	// CAN0_SendData(CAN_sendData);

	OS_Kill();
}


//******** Thread_CAN  *************** 
// Outputs CAN sensor value on ST7735
// never blocks, never sleeps, never dies
void Thread_CAN(void){
	while(1) {
	
		
//		ST7735_Message(1,0, "XMT_ID", XMT_ID);
		ST7735_Message(1,1, "send Lidar0:", CAN_sendData[0]);
		ST7735_Message(1,2, "send Lidar1:", CAN_sendData[1]);
		ST7735_Message(1,3, "send Lidar2", CAN_sendData[2]); 
		ST7735_Message(1,4, "send IR 0", CAN_sendData[3]); 
		ST7735_Message(1,5, "send IR 1", CAN_sendData[4]); 
		
		OS_Suspend();
	}
}


//******** IdleTask  *************** 
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount=0;
void IdleTask(void){ 
  while(1) { 
    Idlecount++;        // debugging 
  }
}


