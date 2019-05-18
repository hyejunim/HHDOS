#include <stdint.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Thread_sSort.h"
#include "Thread_sCAN.h"


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))

//uint16_t arr_size;
//uint8_t arr[MAX_ARRSIZE];

uint16_t arrived_data_num = 0;

//******** Thread_CANGetArrSize  *************** 
// Get Array Size from CAN
// Kills itself after receiving
uint8_t CAN_ArrSize[8];
void Thread_CANGetArrSize(void){
	// Get array size from CAN
	CAN0_GetMailNonBlock(CAN_ArrSize);
	arr_size = CAN_ArrSize[0] + (CAN_ArrSize[1] << 8);
	
	// Now get array data from master
	while(NumCreated == 8){OS_Suspend();}
	NumCreated += OS_AddThread(Thread_CANGetArr8_Sort, 128, 2);
	
	PF1 = 0x02;
	OS_Kill();
}

//******** Thread_CANGetArrSize  *************** 
// Get Array data from CAN and store it into the local array in the right place
// Kills itself after receiving
uint8_t CAN_Arr8[8];
void Thread_CANGetArr8_Sort(void){

	uint8_t tmp_arr[8];
	
	// get data
	CAN0_GetMailNonBlock(tmp_arr);
	
	// count arrived data number
	arrived_data_num += 8;
	if(arrived_data_num < arr_size)	// need more data to receive
	{
		while(NumCreated == 8){OS_Suspend();}
		NumCreated += OS_AddThread(Thread_CANGetArr8_Sort, 128, 2);
	}
	else	// finished receiving and time to merge
	{
		while(NumCreated == 8){OS_Suspend();}
		NumCreated += OS_AddThread(Thread_Merge, 128, 2);
	}
	
	MergeSort(tmp_arr, 8);
	
	PF2 ^= 0x04;
	OS_Kill();
}



//******** Thread_CANSendData  *************** 
// Send Data to CAN
// Kills itself after sending
uint8_t CAN_sendArr[8];
void Thread_CANSendArr(void){
	
	for(int i=0; i<arr_size; i+=8)
	{
		for(int j=0; j<8; j++)
		{
			CAN_sendArr[j] = arr[i*8 + j];
		}
		CAN0_SendData(CAN_sendArr);
	}
	
	while(NumCreated == 8){OS_Suspend();}
	NumCreated += OS_AddThread(Thread_CANGetArrSize, 128, 2);

	OS_Kill();
}

