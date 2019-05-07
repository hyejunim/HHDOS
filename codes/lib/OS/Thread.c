#include <stdint.h>
#include "ST7735.h"
#include "VL53L0X.h"
#include "IR.h"
#include "can0.h"
#include "OS.h"

//uint16_t d;
VL53L0X_RangingMeasurementData_t lidar0;
VL53L0X_RangingMeasurementData_t lidar1;
VL53L0X_RangingMeasurementData_t lidar2;

uint32_t adcL;
uint32_t adcR;




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
		
		
		CAN0_SendData(tXmtData);
		
		ST7735_Message(1,0, "XMT_ID", XMT_ID);
		ST7735_Message(1,1, "send Lidar0:", tXmtData[0]);
		ST7735_Message(1,2, "send Lidar1:", tXmtData[1]);
		ST7735_Message(1,3, "send Lidar2", tXmtData[2]); 
		ST7735_Message(1,4, "send IR 0", tXmtData[3]); 
		ST7735_Message(1,5, "send IR 1", tXmtData[4]); 
		
		OS_Suspend();
	}
}

//******** Thread_Laser_LCD  *************** 
// Outputs lidar sensor value on ST7735
// never blocks, never sleeps, never dies
void Thread_Laser_LCD(void)
{
	while(1)
	{
		VL53L0X_getSingleRangingMeasurement(&lidar0, 0);
		if (lidar0.RangeStatus != 4 || lidar0.RangeMilliMeter < 8000) {
//			d = lidar0.RangeMilliMeter;
			tXmtData[0] = lidar0.RangeMilliMeter/10; //data for Lidar0 (Left Corner)
//			ST7735_Message(0, 0, "Distance(mm): ", d);
			//UART_OutString("Distance(mm): ");UART_OutUDec(measurement.RangeMilliMeter);
		} else {
			tXmtData[0] = 100; //data for Lidar0 (Left Corner)
//				ST7735_Message(0, 0, "Out of range :( ", 0);
			//UART_OutString("Out of range :( ");
		}
		
		
		VL53L0X_getSingleRangingMeasurement(&lidar1, 1);
		if (lidar1.RangeStatus != 4 || lidar1.RangeMilliMeter < 8000) {
//			d = lidar1.RangeMilliMeter;
				tXmtData[1] = lidar1.RangeMilliMeter/10; //data for Lidar0 (Left Corner)
//			ST7735_Message(0, 1, "Distance(mm): ", d);
			//UART_OutString("Distance(mm): ");UART_OutUDec(measurement.RangeMilliMeter);
		} else {
				tXmtData[1] = 100; //data for Lidar0 (Left Corner)
//				ST7735_Message(0, 1, "Out of range :( ", 0);
			//UART_OutString("Out of range :( ");
		}
		
		// LIDAR3
		VL53L0X_getSingleRangingMeasurement(&lidar2, 2);
		if (lidar2.RangeStatus != 4 || lidar2.RangeMilliMeter < 8000) {
				tXmtData[2] = lidar2.RangeMilliMeter/10; //data for Lidar0 (Left Corner)
//           ST7735_Message(0, 2, "Distance(mm): ", lidar2.RangeMilliMeter);
		} else {
				tXmtData[2] = 100; //data for Lidar0 (Left Corner)
//            ST7735_Message(0, 2, "Out of range :( ", 0);
		}
				
	OS_Suspend();			
	}
}


//******** Thread_IR_LCD  *************** 
// Outputs IR sensor value on ST7735
// never blocks, never sleeps, never dies
void Thread_IR_LCD(void)
{
	while(1) {
		uint32_t adc_data = IR_filtered(1);
		adcL = IR_calib(adc_data);
		
		if(adcL >= 60){
			tXmtData[3] = 100;
//			ST7735_Message(0, 3, "IR0 (mm) > 60cm ", 0);
	}
		else{
			tXmtData[3] = adcL;
//			ST7735_Message(0, 3, "IR0 (mm):       ", adcL);
		}
			
		adc_data = IR_filtered(2);
		adcR = IR_calib(adc_data);
		if(adcR >= 60){
			tXmtData[4] = 100;
//			ST7735_Message(0, 4, "IR1 (mm) > 60cm ", 0);
		}
		else{
			tXmtData[4] = adcR;
//			ST7735_Message(0, 4, "IR1 (mm):       ", adcR);
	
		}
		
	OS_Suspend();	
	}
}


int BumperCountL = 0;
int BumperCountR = 0;
//******** Thread_BumperL  *************** 
// Notify bumper value through CAN
void Thread_BumperL(void)
{
	BumperCountL++;
	ST7735_Message(0, 1, "BumperL:", BumperCountL);
	OS_Kill();
}

//******** Thread_BumperR  *************** 
// Notify bumper value through CAN
void Thread_BumperR(void)
{
	BumperCountR++;
	ST7735_Message(0, 1, "BumperR:", BumperCountR);
	OS_Kill();	
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


