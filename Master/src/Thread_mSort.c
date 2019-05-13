#include <stdint.h>
#include <stdlib.h>

#include "ST7735.h"
#include "OS.h"
#include "Thread_mSort.h"
#include "Thread_mCAN.h"
#include "Interpreter.h"
#include "UART2.h"

#define MIN(x, y) (x<y)? x :y


#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))


unsigned long start_time; 
unsigned long stop_time;


unsigned long arr_size;

/*********************Random Input***********************/
void Random_Input (void){
	
	 UART_OutString(" Size of the array? ");
	 arr_size = UART_InUDec();
	 UART_OutCRLF();
		
	
	 UART_OutString(" Start generating random numbers.... ");
	 UART_OutCRLF();
	
		srand(OS_Time()); // to make random array
	
		for(int i =0; i<arr_size; i++){
			arr[i] = rand() % 255;
			UART_OutUDec(arr[i]);
			UART_OutCRLF();
		}

   OS_Kill();
}


/*********************PrintResult***********************/
void Print_Result (void){
	
	 UART_OutString(" Done Sorting.... ");
	 UART_OutCRLF();
	 
		for(int i =0; i<arr_size; i++){
			UART_OutUDec(arr[i]);
			UART_OutCRLF();
		}
		

		
		UART_OutString(" Start time: ");
		UART_OutUDec(start_time);
		UART_OutCRLF();	
		UART_OutString(" End time: ");
		UART_OutUDec(stop_time);
		UART_OutCRLF();	
		UART_OutString(" Total time: ");
		UART_OutUDec(project_OS_TimeDifference(start_time, stop_time));		
		UART_OutCRLF();
		
   OS_Kill();		
}


//int arr[12];
//int arr_size;
//******** Thread_MergeSort  *************** 
// Iterative merge sort
// Kills itself
void Thread_MergeSort(){

   int curr_size;  // For current size of subarrays to be merged 
                   // curr_size varies from 1 to n/2 
   int left_start; // For picking starting index of left subarray 
                   // to be merged 
	
	
//*****start measuring time*******//	
	start_time = OS_Time();  
	
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
		stop_time	= OS_Time();
	 
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
