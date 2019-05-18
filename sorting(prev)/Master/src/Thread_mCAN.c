#include <stdint.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Thread_mSort.h"
#include "Thread_mCAN.h"


//uint16_t arr_size;
//uint8_t arr[MAX_ARRSIZE];

uint16_t arrived_data_num = 0;



void CANSendArrSize(int size)
{
	uint8_t CAN_sendArrSize[8];
	CAN_sendArrSize[0] = size & 0xFF;
	CAN_sendArrSize[1] = (size >> 8) & 0xFF;
	CAN0_SendData1(CAN_sendArrSize);
}


//******** Thread_CANSendData  *************** 
// Send Data to CAN
// Kills itself after sending
uint8_t CAN_sendArr[8];
void Thread_CANSendArr(void){
	int size = 32;
	CANSendArrSize(size);
	
	for(int i=0; i<size/8; i++)
	{
		for(int j=0; j<8; j++)
		{
			CAN_sendArr[j]=j;
		}
		CAN0_SendData1(CAN_sendArr);
	}
	OS_Kill();
}


