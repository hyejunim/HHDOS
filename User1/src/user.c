#include <stdint.h>
#include "user.h"
#include "can0.h"
#include "User_Interpreter.h"
#include "UART2.h"


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))

// Receive data packet from CAN
void Thread_RcvCAN(void)
{
	uint8_t data[8];
	CAN0_GetMail(data);
}


uint8_t state_send = 0;
uint8_t flag_wqsend = 0;
uint8_t flag_qsend = 0;
uint8_t packet8[8] = {0,};
uint16_t idx = 0;
void wqController(void)
{
	if(flag_wqsend)
	{
		switch(state_send)
		{
			// 1) send the first packet with filename
			case 0:	
			idx = 0;
			PF1 = 2;
			for(int i=1; i<8; i++)	packet8[i] = 0;
			
			packet8[0] = WQ;
			for(int i=1; i<8; i++)
				packet8[i] = wqfilename[i-1];
			//state_send = 1;
			break;
			
			
			// 2) send the second packet with filesize
			case 1:	
			PF1 = 2;
			for(int i=1; i<8; i++)	packet8[i] = 0;
			
			packet8[0] = '*';
			for(int i=7; i>0; i--)
				packet8[i] = (wqfilesize >> 8*(7-i)) & 0xFF;
			//state_send = 2;
			break;
			
			case 2:	// 3) send file packets
			PF3 = 8;
			for(int i=1; i<8; i++)	packet8[i] = 0;
			
			for(int i=0; i<8; i++)
			{
				packet8[i] = wqmodified[idx++];
				if(idx == wqfilesize)	
				{
					//state_send = 3;
					break;
				}
			}
			//else 				state_send = 2;
			break;
			
			case 3: 	// 4) wrap up
			PF1 = 0;	PF3 = 0;
			state_send = 0;
			flag_wqsend = 0;
			idx = 0;
			break;
		}
	}
}

//void qController(void)
//{
//	if(flag_qsend)
//	{
//		switch(state_send)
//		{
//			case 0:	// 1) send the first packet with filename
//			PF1 = 0;
//			packet8[0] = WQ;
//			for(int i=1; i<8; i++)
//				packet8[i] = wqfilename[i-1];
//			state_send = 1;
//			break;
//			
//			case 1:	// 2) send the second packet with filesize
//			PF1 = 0;
//			// adjust wqfilesize a bit
//			for(int i=0; i<8; i++)
//				if((wqfilesize - i) % 8 == 0) break;
//			
//			packet8[0] = '*';
//			for(int i=7; i>0; i--)
//				packet8[i] = (wqfilesize >> 8*(7-i)) & 0xFF;
//			state_send = 2;
//			break;
//			
//			case 2:	// 3) send file packets
//			PF3 = 8;
//			for(int i=0; i<8; i--)
//				packet8[i] = wqoriginal[idx++];
//			if(idx == wqfilesize)	state_send = 3;
//			else 				state_send = 2;
//			break;
//			
//			case 3: 	// 4) wrap up
//			PF1 = 0;	PF3 = 0;
//			state_send = 0;
//			flag_qsend = 0;
//			idx = 0;
//			break;
//		}
//		
//	}
//}
void PeriodicSendCAN(void){
	for (int i=0; i<8; i++) packet8[i] = 0;
	wqController();
	//qController();
	
	if(flag_qsend || flag_wqsend)
	switch(state_send)
	{
		case 0:
		CAN0_SendData(packet8);
		
		// wait response from the server
		uint8_t response[8];
		CAN0_GetMail(response);
		state_send = 1;
		break;
		
		case 1:
		CAN0_SendData(packet8);
		state_send = 2;
		break;
		
		case 2:
		CAN0_SendData(packet8);
		if(idx >= wqfilesize){
			PF1 = 0;	PF3 = 0;
			state_send = 0;
			flag_wqsend = 0;
			idx = 0;
			
			// reset all golbal values before quit
			for(int i=0; i<7; i++)	wqfilename[i] = 0;
			for(int i=0; i<FILESIZE; i++)
			{
				wqoriginal[i] = 0;
				wqmodified[i] = 0;
			}
			wqfilesize = 0;
		}
		else			state_send = 2;
		break;
			
		case 3:
		state_send = 0;
		idx = 0;
		break;
	}
	
}

