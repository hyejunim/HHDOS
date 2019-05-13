#include <stdint.h>
#include <stdlib.h>

#include "ST7735.h"
#include "OS.h"
#include "Thread_sort.h"
#include "Interpreter.h"
#include "UART2.h"
#include "can0.h"


#define MIN(x, y) (x<y)? x :y


#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))


unsigned long start_time; 
unsigned long stop_time;


uint16_t arr_size;
int arr[256] = {0};

uint8_t s_init_data[8];
uint8_t sdata[8]; 
uint32_t snum=0;



int final_result[256] = {0};

/********************* Reset ***********************/
void Master_Reset (void){
	
	PF3 = 0x00;
	PF1 = 0x02;
	
	for(int i=0; i<arr_size; i++){
		arr[i] = 0;
	}
	snum = 0;
	//rnum = 0;
	
	UART_OutCRLF();UART_OutCRLF();
	UART_OutString(" /*************START*************/"); UART_OutCRLF();
		
	NumCreated += OS_AddThread(Random_Input, 128, 2); // calls CAN_Send (priority 1)
	NumCreated += OS_AddThread(CAN_Receive, 128, 3);
	NumCreated += OS_AddThread(Print_Result, 128, 4);
		
}


/********************* CAN_Send ***********************/
// normally sending arr[] to Slave1 by CAN
void CAN_Send (void){
	
	UART_OutString(" Sending numbers.... "); UART_OutUDec(snum); UART_OutCRLF();
	for(int i=0; i<8; i++){
		sdata[i] = arr[i+snum*8];
		UART_OutUDec(sdata[i]); UART_OutCRLF();
	}
  CAN0_SendData1(sdata);
	
	snum++;
	
	OS_Kill();
}




/*********************Random Input***********************/
// Generate random input, arr[]
void Random_Input (void){
	
	 //UART_OutString(" Size of the array? "); 
	 //arr_size = UART_InUDec(); UART_OutCRLF(); 
	arr_size =32;
	
	UART_OutUDec(arr_size); UART_OutCRLF();
	//send arr_size to Slave1
	s_init_data[0] = arr_size & 0x00FF;
	s_init_data[1] = (arr_size & 0xFF00) >>8;	
  CAN0_SendData1(s_init_data);
	
//	 UART_OutString(" Start generating random numbers.... "); UART_OutCRLF();
	 srand(OS_Time()); // to make random array
	
	
//		UART_OutString(" Send: "); UART_OutCRLF();	
	 for(int j=0; j<(arr_size/8); j++){
			for(int i =0; i<8; i++){
				arr[ i + j*8 ] = rand() % 255;
				sdata[i] = arr[i+j*8];
				UART_OutUDec(sdata[i]); UART_OutCRLF();
			}
//			NumCreated += OS_AddThread(CAN_Send, 128, 1);
			CAN0_SendData1(sdata);
			
		}
		PF1 =0x00;
		OS_Kill();
}



uint8_t rdata[8];
uint32_t rnum=0;
/********************* CAN_Receive ***********************/
void CAN_Receive (void){
//	rnum = 0;
 while(rnum < (arr_size/8)){
		PF2 =0x04;
    if(CAN0_GetMailNonBlock(rdata)){
			
			
		UART_OutString(" Receiving numbers.... "); UART_OutCRLF();
			for(int i=0; i<8; i++){
				final_result[i + rnum*8] = rdata[i];
				UART_OutUDec(rdata[i]); UART_OutCRLF();
				
			}
			rnum ++;
    }
	}
	PF1 =0x00;
	OS_Kill();
}
/*********************PrintResult***********************/
void Print_Result (void){
	
	 UART_OutString(" Done Sorting.... "); UART_OutCRLF();
	 
	for(int i =0; i<arr_size; i++){
			UART_OutUDec(final_result[i]); UART_OutCRLF();
	}
	
/*	
		UART_OutString(" Start time: "); UART_OutUDec(start_time); UART_OutCRLF();	
		UART_OutString(" End time: "); UART_OutUDec(stop_time); UART_OutCRLF();	
		UART_OutString(" Total time: "); UART_OutUDec(project_OS_TimeDifference(start_time, stop_time)); UART_OutString(" us"); UART_OutCRLF();
*/		
	
   OS_Kill();		
}



//******** Thread_MergeSort  *************** 
// Iterative merge sort
// Kills itself
void Thread_MergeSort(){

   int curr_size;  // For current size of subarrays to be merged 
                   // curr_size varies from 1 to n/2 
   int left_start; // For picking starting index of left subarray 
                   // to be merged 
	
	
//*****start measuring time*******//	
	start_time = OS_usTime();  
	
	UART_OutString(" Start Sorting....");
	UART_OutCRLF();
 
	
   // Merge subarrays in bottom up manner.  First merge subarrays of 
   // size 1 to create sorted subarrays of size 2, then merge subarrays 
   // of size 2 to create sorted subarrays of size 4, and so on. 
   for (curr_size=1; curr_size<=arr_size-1; curr_size = 2*curr_size) 
   { 
       // Pick starting point of different subarrays of current size 
       for (left_start=0; left_start<arr_size-1; left_start += 2*curr_size) 
       { 
           // Find ending point of left subarray. mid+1 is starting  
           // point of right 
           int mid = left_start + curr_size - 1; 
  
           int right_end = MIN(left_start + 2*curr_size - 1, arr_size-1); 
  
           // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end] 
           Merge(arr, left_start, mid, right_end); 
       }
   }
	 
	 
//*****done measuring time*******//	
		stop_time	= OS_usTime();
	 
//*****Print result*******//	
	 NumCreated += OS_AddThread(Print_Result, 128, 1);

   OS_Kill();
}

//******** Merge  *************** 
// Merges two arrays
void Merge(int arr[], int l, int m, int r){
	
    int i, j, k; 
    int n1 = m - l + 1; 
    int n2 =  r - m; 
  
    /* create temp arrays */
    int L[n1], R[n2]; 
  
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++) 
        L[i] = arr[l + i]; 
    for (j = 0; j < n2; j++) 
        R[j] = arr[m + 1+ j]; 
  
    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; 
    j = 0; 
    k = l; 
    while (i < n1 && j < n2) 
    { 
        if (L[i] <= R[j]) 
        { 
            arr[k] = L[i]; 
            i++; 
        } 
        else
        { 
            arr[k] = R[j]; 
            j++; 
        } 
        k++; 
    } 
  
    /* Copy the remaining elements of L[], if there are any */
    while (i < n1) 
    { 
        arr[k] = L[i]; 
        i++; 
        k++; 
    } 
  
    /* Copy the remaining elements of R[], if there are any */
    while (j < n2) 
    { 
        arr[k] = R[j]; 
        j++; 
        k++; 
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
		PF3 ^= 0x08;
    Idlecount++;        // debugging 
  }
}

