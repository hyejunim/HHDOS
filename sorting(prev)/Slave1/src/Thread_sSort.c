#include <stdint.h>
#include <stdlib.h>

#include "ST7735.h"
#include "OS.h"
#include "Thread_sSort.h"
#include "Thread_sCAN.h"
#include "Interpreter.h"
#include "UART2.h"

#define MIN(x, y) (x<y)? x :y


#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PF4       (*((volatile uint32_t *)0x40025040))


unsigned long start_time; 
unsigned long stop_time;


uint16_t arr_size = 0;
uint8_t arr[MAX_ARRSIZE] = {0,};
uint16_t arr_8sorted_idx = 0;


void create_testArr()
{
	arr_size = 32;
	arr[0] = 2;
	arr[1] = 43;
	arr[2] = 6;
	arr[3] = 56;
	arr[4] = 35;
	arr[5] = 37;
	arr[6] = 122;
	arr[7] = 50;
	arr[8] = 69;
	arr[9] = 46;
	arr[10] = 89;
	arr[11] = 32;
	arr[12] = 41;
	arr[13] = 212;
	arr[14] = 111;
	arr[15] = 46;
	arr[16] = 3;
	arr[17] = 4;
	arr[18] = 90;
	arr[19] = 70;
	arr[20] = 100;
	arr[21] = 97;
	arr[22] = 45;
	arr[23] = 87;
	arr[24] = 59;
	arr[25] = 43;
	arr[26] = 34;
	arr[27] = 12;
	arr[28] = 21;
	arr[29] = 32;
	arr[30] = 49;
	arr[31] = 72;
}




//******** MergeSort  *************** 
// Iterative merge sort function of an array number of size
void MergeSort(uint8_t* pArr, uint8_t size){

	int curr_size;  // For current size of subarrays to be merged 
					// curr_size varies from 1 to n/2 
	int left_start; // For picking starting index of left subarray 
				   // to be merged 
	
	// Merge subarrays in bottom up manner.  First merge subarrays of 
	// size 1 to create sorted subarrays of size 2, then merge subarrays 
	// of size 2 to create sorted subarrays of size 4, and so on. 
	for (curr_size=1; curr_size<=size-1; curr_size = 2*curr_size) 
	{ 
	   // Pick starting point of different subarrays of current size 
	   for (left_start=0; left_start<size-1; left_start += 2*curr_size) 
	   { 
		   // Find ending point of left subarray. mid+1 is starting  
		   // point of right 
		   int mid = left_start + curr_size - 1; 

		   int right_end = MIN(left_start + 2*curr_size - 1, size-1); 

		   // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end] 
		   Merge(pArr, left_start, mid, right_end); 
	   }
	}
	
	
	// Fill the array after the sorting
	for(int i=0; i<size; i++)
		arr[arr_8sorted_idx + i] = pArr[i];
	
	arr_8sorted_idx += size;

}

uint16_t merge_granularity = 8;
void Thread_Merge()
{
	while(merge_granularity < arr_size)
	{
		for(int i=0; i<arr_size; i+=2*merge_granularity)
		{
			Merge(arr, i, i+merge_granularity-1, i+2*merge_granularity-1);
		}
		merge_granularity *= 2;
	}

	while(NumCreated == 8){OS_Suspend();}
	NumCreated += OS_AddThread(Thread_CANSendArr, 128, 2);

	PF3 ^= 0x08;
	OS_Kill();
}

//******** Merge  *************** 
// Merges two arrays
void Merge(uint8_t arr[], int l, int m, int r){
	
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
