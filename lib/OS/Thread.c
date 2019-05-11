#include <stdint.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"




//******** Thread_CAN  *************** 
// Outputs CAN sensor value on ST7735
// never blocks, never sleeps, never dies
uint8_t tXmtData[5];
void Thread_CAN(void){
	while(1) {
	
//		tXmtData[1] = lidar1.RangeMilliMeter/10; //data for Lidar1 (Center)
//		tXmtData[2] = lidar2.RangeMilliMeter/10;  //data for Lidar2 (Right Corner)
//		tXmtData[3] = adcL;  //data for IR0  (Right)
//		tXmtData[4] = adcR;  //data for IR1 (Left)
		
		
//		CAN0_SendData(tXmtData);
		
//		ST7735_Message(1,0, "XMT_ID", XMT_ID);
		ST7735_Message(1,1, "send Lidar0:", tXmtData[0]);
		ST7735_Message(1,2, "send Lidar1:", tXmtData[1]);
		ST7735_Message(1,3, "send Lidar2", tXmtData[2]); 
		ST7735_Message(1,4, "send IR 0", tXmtData[3]); 
		ST7735_Message(1,5, "send IR 1", tXmtData[4]); 
		
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


