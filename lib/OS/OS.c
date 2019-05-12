/////////////////////////////////////////////////////////////////////////////////////////////////////////
// *************os.c**************
// Hyejun Im (hi956), Hyunsu Chae (hc25673)
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* TIMER CHECKOUT LIST
		Timer0: OS time measurement
		Timer1: 
		Timer2: ADC
		Timer3: CAN
		WTimer4: OS background process
		Timer5: 
	*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>
#include <stdlib.h>
#include "OS.h"
#include "PLL.h"
#include "ST7735.h"
#include "hw_types.h"
#include "tm4c123gh6pm.h"
#include "UART2.h"
#include "Interpreter.h"

#define Lab2 0
#define Lab3 1
#define check_timestamp 0

#define PRI_MAX 8
#define OS_TIME_PERIOD 80000 // 1ms

#define PE1  (*((volatile unsigned long *)0x40024008))


//******prototypes from startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical(void);     // save previous I bit, disable interrupts, return previous I
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// prototypes from OSasm.s
void ContextSwitch(void);
void StartOS(void);
void PendSV_Handler(void);

// main threads
#define THREAD_SIZE  8        // maximum number of threads
#define STACK_SIZE   256      // number of 32-bit words in stackTCB* thread
TCB thread[THREAD_SIZE];

//For active threads
TCB* active_start;
TCB* runPt;
TCB* runPt_next;
uint16_t thread_cnt = 0;
unsigned long active_num=0; // count number of active thread
Sema4Type sActive_num;

//For active threads with priority
TCB* pri_start[PRI_MAX];
unsigned long pri_num[PRI_MAX] ={0};
unsigned long blocked_pri_num[PRI_MAX] ={0};

//For sleeping threads
TCB* sleepPt;
TCB* sleep_start;
unsigned long sleep_num=0; // count number of active thread
Sema4Type sSleep_num;


// functions for periodic tasks
void (*PeriodicTask1)(void);   // user function
void (*PeriodicTask2)(void);   // user function
int PeriodicTask_num = 0;
unsigned int PTask1_Period;	// time measurement
unsigned int PTask2_Period;	// time measurement

// switch tasks
void (*sw1task)(void);
void (*sw2task)(void);
unsigned long sw1_priority;
unsigned long sw2_priority;

// FIFO
#define FIFOSIZE  128 // change_here
uint32_t volatile *putPt;
uint32_t volatile *getPt;
uint32_t static Fifo[FIFOSIZE];
Sema4Type FIFOcurrentSize;
Sema4Type FIFOmutex; 
Sema4Type FIFOroomleft;

// Mailbox
unsigned long Mail;
Sema4Type Send;
Sema4Type Ack; 

int OS_Launch_Check = 0;
int num1234 = 0;
unsigned long no_interrupt_time = 0;

#if check_timestamp
//For TimeStamps
long TimeStamp[100];
char* TimeMessage[100];
int time_index = 0;
int timestamp_done = 0;
#endif 

#if Lab3
unsigned long MaxJitter1;    
unsigned long MaxJitter2;   
#define JITTERSIZE 64
unsigned long const JitterSize=JITTERSIZE;
unsigned long JitterHistogram1[JITTERSIZE]={0,};
unsigned long JitterHistogram2[JITTERSIZE]={0,};
#endif


#define PF0             (*((volatile uint32_t *)0x40025004))	// SW2
#define PF1             (*((volatile uint32_t *)0x40025008))	// LED
#define PF2             (*((volatile uint32_t *)0x40025010))	// LED
#define PF3             (*((volatile uint32_t *)0x40025020))	// LED (SW1 task)
#define PF4             (*((volatile uint32_t *)0x40025040))	// SW1
#define SW1			0x10
#define SW2			0x01
#define LEDS      (*((volatile uint32_t *)0x40025038))
#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08

#define PE0  (*((volatile unsigned long *)0x40024004))
#define PE1  (*((volatile unsigned long *)0x40024008))
#define PE2  (*((volatile unsigned long *)0x40024010))
#define PE3  (*((volatile unsigned long *)0x40024020))





void PortF_Init(void){ 
	
  SYSCTL_RCGCGPIO_R |= 0x20;       // activate port F
  while((SYSCTL_PRGPIO_R&0x20) == 0){}			// ready?
		
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;  	// 2a) unlock GPIO Port F Commit Register
  GPIO_PORTF_CR_R |= (SW1|SW2);  						// 2b) enable commit for PF4 and PF0
  GPIO_PORTF_DIR_R &= ~(SW1|SW2);						// 5) make PF0 and PF4 in (built-in buttons)
  GPIO_PORTF_AFSEL_R &= ~(SW1|SW2); 		  // 6) disable alt funct on PF0 and PF4
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFF0FFF0)+0x00000000;      // 4) configure PF0 and PF4 as GPIO
  GPIO_PORTF_PUR_R |= (SW1|SW2); 						// enable weak pull-up on PF0 and PF4
  GPIO_PORTF_AMSEL_R &= ~(SW1|SW2);    	// 3) disable analog functionality on PF4 and PF0
  GPIO_PORTF_DEN_R |= (SW1|SW2);						// 7) enable digital I/O on PF0 and PF4
	GPIO_PORTF_IM_R = 0x11;							// enable interrupt for PF0 and PF4
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF0,4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF0,4 is not both edges
  GPIO_PORTF_IEV_R |= 0x11;    //     PF0,4 rising edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag0,4
	
	// for PortF: vector number 46, interrupt number 30
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF) | (5<<16); // 8) set priority = 3: PRI7 is for interrupt 28~31
	NVIC_EN0_R = 1<<30;   // 9) enable: EN0_R is for interrupt 0-31

		
		
	// LED turning on config
	GPIO_PORTF_DIR_R |= (RED|BLUE|GREEN); 
  GPIO_PORTF_AFSEL_R &= ~(RED|BLUE|GREEN);// disable alt funct on PF3-1
  GPIO_PORTF_DEN_R |= (RED|BLUE|GREEN); // enable digital I/O on PF3-1
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF000F)+0x00000000; // configure PF3-1 as GPIO
  GPIO_PORTF_AMSEL_R = 0;     // disable analog functionality on PF
	LEDS = 0;                        // turn all LEDs off
		

}

void PortE_Init(void){ 
  SYSCTL_RCGCGPIO_R |= 0x10;       // activate port E
  while((SYSCTL_RCGCGPIO_R&0x10)==0){};      
  GPIO_PORTE_DIR_R |= 0x0F;    // make PE3-0 output heartbeats
  GPIO_PORTE_AFSEL_R &= ~0x0F;   // disable alt funct on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     // enable digital I/O on PE3-0
  GPIO_PORTE_PCTL_R = ~0x0000FFFF;
  GPIO_PORTE_AMSEL_R &= ~0x0F;;      // disable analog functionality on PF
}




/*********OSWTimer4_Init**********/
// Initialize WTimer4 for PeriodicThreads
void OSWTimer4_Init(void)
{
	SYSCTL_RCGCWTIMER_R |= 0x10;   // activate wide_timer4
	while((SYSCTL_RCGCWTIMER_R & 0x10) == 0) {}
		
  WTIMER4_CTL_R = 0x00000000;    // disable timer4A during setup
//	WTIMER4_CFG_R = 0x00000000; // configure for 64-bit timer mode 	
	WTIMER4_CFG_R = 0x00000004;  //    (0311)
 // WTIMER4_TAMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
	WTIMER4_TAMR_R = 0x00000082;   //(0311)
  WTIMER4_TAPR_R = 0;            // prescale value for trigger
 // WTIMER4_TBMR_R = 0x00000002;   // configure for periodic mode, default down-count settings
	WTIMER4_TBMR_R = 0x00000082;   // (0311)
  WTIMER4_TBPR_R = 0;            // prescale value for trigger
}


/*********OSTimer0_Init**********/
// Initialize Timer0 for OS TIme 
void OSTimer0_Init(void)
{
	DisableInterrupts();
  SYSCTL_RCGCTIMER_R |= 0x01;   // 0) activate TIMER0
	while((SYSCTL_RCGCTIMER_R & 0x01) == 0) {}
  TIMER0_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER0_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
	TIMER0_TAMR_R = 0x00000002;   // 3) configure for periodic mode, count up settings,snapshot
  TIMER0_TAILR_R = OS_TIME_PERIOD;    // 4) reload value 

	TIMER0_TAPR_R = 0;            // 5) bus clock resolution
  TIMER0_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000001;    // 7) arm timeout interrupt
		
//  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
//  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  TIMER0_CTL_R = 0x00000001;    // 10) enable TIMER0A
 // EnableInterrupts();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////here2
/********* OS_AddPeriodicThread **********/
// system timer driver using 32-bit periodic timer interrupts
// Run the "task" at every "period" milisecond
/*input: task: pointer to the function
       period: (milisecond)
	     priority: value to be specified in the NVIC
		output: 1 if successful, 0 if this thread can not be added
*/
//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////here1
int OS_AddPeriodicThread(void(*task)(void), unsigned long period, unsigned long priority)
{
	int32_t status = StartCritical();
#if check_timestamp	
	unsigned long start_time = OS_MsTime(); // time measurements
	unsigned long stop_time;
#endif
	
	if(PeriodicTask_num == 0)
	{
		PeriodicTask1 = task;          // user function
		PeriodicTask_num++;
		
		PTask1_Period = period -1;
		WTIMER4_TAILR_R = period-1;    		// Timer A Interval Load 4) reload value
    WTIMER4_ICR_R |= 0x00000001;    // 6) clear WTIMER0A timeout flag
		WTIMER4_IMR_R |= 0x00000001;    	// 7) arm timeout interrupt
		
		// for WTimer4A: vector number 118, interrupt number 102
		NVIC_PRI25_R |= (NVIC_PRI25_R&0xFF00FFFF) | (priority<<16); // 8) set priority: PRI25 is for interrupt 100~103
		NVIC_EN3_R |= 1<<6;   // 9) enable: EN3_R is for interrupt 96~127 (102-96=6)
		if(OS_Launch_Check){
			WTIMER4_CTL_R |= 0x00000001;    // 10) enable wtimer4A
		}
#if check_timestamp	
		stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);		
#endif
		EndCritical(status);
		return 1; //add sucessful
	}
	else if(PeriodicTask_num == 1)
	{
		PeriodicTask2 = task;          // user function
		PeriodicTask_num++;
		
		PTask2_Period = period -1;
		WTIMER4_TBILR_R = period-1;    		// Timer B Interval Load 4) reload value
    WTIMER4_ICR_R |= 0x00000100;    // 6) clear WTIMER0B timeout flag
		WTIMER4_IMR_R |= 0x00000100;    	// 7) arm timeout interrupt

		// for WTimer4B: vector number 119, interrupt number 103
		NVIC_PRI25_R |= (NVIC_PRI25_R&0x00FFFFFF) | (priority<<24); // 8) set priority: PRI17 is for interrupt 68~71
		NVIC_EN3_R |= 1<<7;   // 9) enable: EN3_R is for interrupt 96~127 (103-96=7)
		
		if(OS_Launch_Check){
			WTIMER4_CTL_R |= 0x00000100;    // 10) enable wtimer4B
		}
#if check_timestamp
		stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
		EndCritical(status);
		return 1; //add sucessful
	}
	else
	{
#if check_timestamp
		stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
		EndCritical(status);
		return 0;
	}
}	
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////here2
#if Lab3
uint32_t total_numJ1=0;
uint32_t total_numJ2=0;
uint32_t sum_J1=0;
uint32_t sum_J2=0;
#endif

int toggle = 0;
uint32_t ms = 0;
/********* WideTimer4A_Handler **********/     
void WideTimer4A_Handler(void){
  WTIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge WTIMER4A timeout
  (*PeriodicTask1)();                // execute user task
	toggle ^= 1;
	ms += toggle;
	
}

/********* WideTimer4B_Handler **********/     
void WideTimer4B_Handler(void){
  WTIMER4_ICR_R = TIMER_ICR_TBTOCINT;// acknowledge TIMER4A timeout
  (*PeriodicTask2)();                // execute user task
	
	toggle ^= 1;
	ms += toggle;
}

/********* OS_ClearPeriodicTime **********/
// Reset the 32-bit counter to 0

void OS_ClearPeriodicTime(void)
{
	WTIMER4_TAILR_R = 0;    // start value for trigger
	WTIMER4_TBILR_R = 0;    // start value for trigger
}

/********* OS_ReadPeriodicTime **********/
//Return the current 32-bit global count
uint32_t OS_ReadPeriodicTime(void)
{
	return WTIMER4_TAR_R; 
}

void Thread_Init(void){
	int i;
	OS_InitSemaphore(&sActive_num, 1);
	OS_InitSemaphore(&sSleep_num, 1);
	for(i=0; i<THREAD_SIZE; i++){
		thread[i].status = 3;
	}
	
}

/*****************************************LAB2,3**************************************/
void OS_Init(void)
{
	DisableInterrupts();
	//PortE_Init(); 
	//PortF_Init();	
	
//	OSWTimer4_Init(); //for AddPeriodicThread
	OSTimer0_Init(); //for OS time measurement
	OS_Launch_Check = 0;
	
	Thread_Init();
	Interpreter_Init();
	OS_MailBox_Init();
	OS_Fifo_Init(256);
	
	UART_Init();
}	 

void Init_Stack(int index)
{

	thread[index].sp = &thread[index].stack[STACK_SIZE-16]; // thread stack pointer
	
  thread[index].stack[STACK_SIZE-1] = 0x01000000;   // thumb bit
  thread[index].stack[STACK_SIZE-3] = 0x14141414;   // R14
  thread[index].stack[STACK_SIZE-4] = 0x12121212;   // R12
  thread[index].stack[STACK_SIZE-5] = 0x03030303;   // R3
  thread[index].stack[STACK_SIZE-6] = 0x02020202;   // R2
  thread[index].stack[STACK_SIZE-7] = 0x01010101;   // R1
  thread[index].stack[STACK_SIZE-8] = 0x00000000;   // R0
  thread[index].stack[STACK_SIZE-9] = 0x11111111;   // R11
  thread[index].stack[STACK_SIZE-10] = 0x10101010;  // R10
  thread[index].stack[STACK_SIZE-11] = 0x09090909;  // R9
  thread[index].stack[STACK_SIZE-12] = 0x08080808;  // R8
  thread[index].stack[STACK_SIZE-13] = 0x07070707;  // R7
  thread[index].stack[STACK_SIZE-14] = 0x06060606;  // R6
  thread[index].stack[STACK_SIZE-15] = 0x05050505;  // R5
  thread[index].stack[STACK_SIZE-16] = 0x04040404;  // R4
}

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority)
{
	int32_t status = StartCritical();
#if check_timestamp
	unsigned long start_time = OS_MsTime(); // time measurements
	unsigned long stop_time;
#endif	
	int i, index;
	
	// find empty index
	for(i=0; i<THREAD_SIZE; i++){
		if(thread[i].status == 3) {
			index = i;
			break;
		}
	}
	
	// thread array is full
	if(i==THREAD_SIZE){
#if check_timestamp
		stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
		EndCritical(status);
		return 0;
	}
	
  // stack init
	Init_Stack(index);
	thread[index].stack[STACK_SIZE-2] = (int32_t)(task); // store PC in stack
	
#if Lab2
	// init TCB
	thread[index].id = thread_cnt++;		// thread id
	thread[index].sleep_cnt = 0;					// sleep counter
	thread[index].age = 0; 									// how long this thread has been active
	thread[index].priority = priority;			// priority
	thread[index].status = 1; 							// status 0: Run, 1: Active, 2: Sleep, 3: Dead, 4: Blocked(not on the slide)
	
	
	// link TCBs (tcb.next)
	if(active_num == 0){ // tcb1 is the only member
		thread[index].next = &thread[index];
		thread[index].prev = &thread[index];

		active_start = &thread[index];	// set active_start
		runPt = active_start;									// set runPt
	} 
	else{
		thread[index].next = active_start;
		thread[index].prev = active_start->prev;
		active_start->prev->next = &thread[index];
		active_start->prev = &thread[index];
	}
	
	// increase active num
	OS_Wait(&sActive_num);
	active_num++;
	OS_Signal(&sActive_num);
		
	// set runPt_next
	runPt_next = runPt ->next;
#if check_timestamp
	stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
	EndCritical(status);

#endif

#if Lab3	
	// init TCB
	thread[index].id = thread_cnt;		// thread id
	thread[index].sleep_cnt = 0;					// sleep counter
	thread[index].age = 0; 									// how long this thread has been active
	thread[index].priority = priority;			// priority
	thread[index].status = 1; 							// status 0: Run, 1: Active, 2: Sleep, 3: Dead, 4: Blocked(not on the slide)
	
	if(pri_num[priority] == 0){ // thread[index] is the only member with this priority
		thread[index].next = &thread[index];
		thread[index].prev = &thread[index];

		pri_start[priority] = &thread[index];	// set pri_start of that priority
		
		 // else, we dont change the runPt
	} 
	else{ // if there is already a thread with the same priority
		// connect the linked list
		thread[index].next = pri_start[priority];
		thread[index].prev = pri_start[priority]->prev;
		pri_start[priority]->prev->next = &thread[index];
		pri_start[priority]->prev = &thread[index];
	}
	thread_cnt++;
	
	// increase active num
	OS_Wait(&sActive_num);
	pri_num[priority]++;
	OS_Signal(&sActive_num);


	// set runPt_next
	runPt_next = runPt ->next;
#if check_timestamp
	stop_time	= OS_MsTime();	// timing purpose
	no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
	
	EndCritical(status);
	
#endif	
	return 1;
}
//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 12.5ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
// It is ok to limit the range of theTimeSlice to match the 24-bit SysTick
void OS_Launch(unsigned long theTimeSlice)
{
	OS_Launch_Check = 1;

#if check_timestamp
	if(!timestamp_done){
		TimeMessage[time_index]= "OSLaunch: ";
		TimeStamp[time_index++] = (0xFFFFFFFF-OS_Time())/80000;
		if (time_index==100)  timestamp_done = 1;
	}
#endif
	
	NVIC_ST_CTRL_R = 0; //disable SysTick during setup
	NVIC_ST_CURRENT_R = 0; // any write to current clears it
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0xE0000000; // SisTick priority 7
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0xFF00FFFF) | 0x00E00000; // PendSV priority 7 
	NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
	NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////here1
	if(PeriodicTask_num == 1){
		WTIMER4_CTL_R |= 0x00000001;    // 10) enable wtimer4A
	}
	else if(PeriodicTask_num == 2){
		WTIMER4_CTL_R |= 0x00000001;    // 10) enable wtimer4A
		WTIMER4_CTL_R |= 0x00000100;    // 10) enable wtimer4B
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////here2
	
	#if Lab3
	//added to check for the highest priority thread, and set it as runPt
	int i;
	int initial_pri;
	
	for(i = 0 ; i<PRI_MAX; i++){	
		if(pri_num[i] != 0){ 
			initial_pri = i;
			break;
		}
	}
	runPt = pri_start[initial_pri];	
	//runPt_next = runPt ->next;
	#endif			
	
  StartOS();
}

// Systick handler
// disable interrupt -> context switch, change runpointer -> enable I
void SysTick_Handler(void)
{
	uint32_t status = StartCritical();
#if check_timestamp
	unsigned long start_time = OS_MsTime(); // time measurements
	unsigned long stop_time;
#endif
	
	int sleep_num_left = sleep_num;

#if Lab2	
	// decrease sleep_cnt
	if(sleep_num > 0) // if there is any thread that is sleeping
	{
		sleepPt = sleep_start;
		
		while(sleep_num_left--)
		{
			if(sleepPt->sleep_cnt <= 0 && sleepPt->status == 2) // wake up
			{
				TCB* curr_sleep = sleepPt;
				TCB* curr_sleep_next = sleepPt->next;
				TCB* curr_sleep_prev = sleepPt->prev;
				
				// update TCB status
				sleepPt->status = 1;
				sleepPt->sleep_cnt = 0;
				
				// disconnect from sleeping list, connect sleeping threads
				sleepPt->prev->next = curr_sleep_next;
				sleepPt->next->prev = curr_sleep_prev;
				
				// connect to active list
				sleepPt->prev = runPt->prev;
				sleepPt->next = runPt;
				runPt->prev->next = curr_sleep;
				runPt->prev = curr_sleep;
				
				// sleep_start is waking up
				if(curr_sleep == sleep_start){
				  sleep_start = curr_sleep_next;
				}
				
				// move sleepPt
				sleepPt = curr_sleep_next;
				
				
				// decrease sleep num
				OS_Wait(&sSleep_num);
				sleep_num--;
				OS_Signal(&sSleep_num);
				
				// increase active num
				OS_Wait(&sActive_num);
				active_num++;
				OS_Signal(&sActive_num);
			}
			else	// decrease sleep_cnt
			{
				(sleepPt->sleep_cnt) -= 2;
				if(sleepPt->sleep_cnt < 0) sleepPt->sleep_cnt = 0;
				
				 sleepPt = sleepPt->next;
			}
			
		}
		
	}
	
#endif
#if Lab3
	
	int sleep_cur_pri;
	
	if(sleep_num > 0) // if there is any thread that is sleeping
	{
		sleepPt = sleep_start; 
		
		while(sleep_num_left--) // let's start checking all sleeping threads
		{
			if(sleepPt->sleep_cnt <= 0 && sleepPt->status == 2) // wake up sleepPt
			{
				TCB* curr_sleep = sleepPt;
				TCB* curr_sleep_next = sleepPt->next;
				TCB* curr_sleep_prev = sleepPt->prev;
				
				// update TCB status
				sleepPt->status = 1;
				sleepPt->sleep_cnt = 0;
				
				
				// disconnect from sleeping list, connect sleeping threads
				sleepPt->prev->next = curr_sleep_next;
				sleepPt->next->prev = curr_sleep_prev;
				
				// connect to active priority list
				sleep_cur_pri = sleepPt->priority;
				
				if(pri_num[sleep_cur_pri] == 0){	// if this waking up thread will be the only thread
					pri_start[sleep_cur_pri] = sleepPt;
					sleepPt->next = sleepPt;
					sleepPt->prev = sleepPt;
				}
				else{ // if there is other thread 
					// connect the linked list
					sleepPt->prev = pri_start[sleep_cur_pri]->prev;
					sleepPt->next = pri_start[sleep_cur_pri];
					
					pri_start[sleep_cur_pri]->prev->next = curr_sleep;
					pri_start[sleep_cur_pri]->prev = curr_sleep;
				
				}
				
				// sleep_start is waking up
				if(curr_sleep == sleep_start){
				  sleep_start = curr_sleep_next;
				}
				
				// move sleepPt
				sleepPt = curr_sleep_next;
				
				
				// decrease sleep num
				OS_Wait(&sSleep_num);
				sleep_num--;
				OS_Signal(&sSleep_num);
				
				// increase active num
				OS_Wait(&sActive_num);
				pri_num[sleep_cur_pri]++;
				OS_Signal(&sActive_num);
			}
			else	// decrease sleep_cnt
			{
				(sleepPt->sleep_cnt) -= 2;
				if(sleepPt->sleep_cnt < 0) sleepPt->sleep_cnt = 0;
				
				 sleepPt = sleepPt->next;
			}
			
		}
		
	}
	
	
	
	
	int i;
	int cur_pri = runPt->priority;
	int next_pri = runPt->priority;
	
	for(i = 0; i<cur_pri; i++){	
		if((pri_num[i] != 0) && (pri_start[i]->status ==1)){
			next_pri = i;
			break;
		}
	}
	if(i == cur_pri){ // meaning that there is no higher priority thread
		runPt_next = runPt->next;
	}
	else{ // meaning that there is a higher priority thread
		runPt_next = pri_start[next_pri];		
	}
	
	
	#endif

#if check_timestamp
	if(!timestamp_done){
		TimeMessage[time_index]= "Systick: ";
		TimeStamp[time_index++] = (0xFFFFFFFF-OS_Time())/80000;
		if (time_index==100)  timestamp_done = 1;
	}
#endif

  NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV (maybe |= is better...?)
#if check_timestamp
		stop_time	= OS_MsTime();	// timing purpose
		no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
	EndCritical(status);

}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void)
{
	
#if Lab3	
	//added to check for the highest priority thread, and set it as runPt
	int i;
	int cur_pri = runPt->priority;
	int next_pri = runPt->priority;

	if(pri_num[cur_pri]-blocked_pri_num[cur_pri] == 0){ // if this suspending thread is the only thread with this priority
		 // find the highest priority active thread to run
		 for(i = 0; i<PRI_MAX; i++){	
			if((pri_num[i] -blocked_pri_num[i]) > 0 && pri_start[i]->status !=2){ // for this, we have to make sure that pri_start is never sleeping
				next_pri = i;
				break;
			}
		}
	
		runPt_next = pri_start[next_pri];
				
	}
	else{ // if there is a thread with the same priority as suspending one
		runPt_next = runPt->next;
	}
#endif
	
  NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV (maybe |= is better...?)
}
 
// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime)
{
		int32_t status = StartCritical();	
#if check_timestamp
	unsigned long start_time = OS_MsTime(); // time measurements
	unsigned long stop_time;
#endif	
	// set sleeping threads
	sleepPt = runPt;

#if Lab2
	//chage runPt_next
	runPt_next = runPt->next;
	
	// reconnect active TCBs
	runPt->next->prev = runPt->prev;
	runPt->prev->next = runPt->next;

	runPt->status = 2;
	runPt->sleep_cnt = sleepTime;
	
	// connect sleeping threads 
	if(sleep_num == 0)
	{
		sleep_start = sleepPt;	// set sleepPt
		
		sleepPt->next = sleepPt;
		sleepPt->prev = sleepPt;
	}
	else
	{
		sleepPt->next = sleep_start;
		sleepPt->prev = sleep_start->prev;
		
		sleepPt->prev->next = sleepPt;
		sleepPt->next->prev = sleepPt;
	}
	
	
	// update cnts
	OS_Wait(&sActive_num);
	active_num--;
	OS_Signal(&sActive_num);
	
	OS_Wait(&sSleep_num);
	sleep_num++;
	OS_Signal(&sSleep_num);
#endif
#if Lab3
	

	
	int i=0;
	int cur_pri = runPt->priority;
	int next_pri = runPt->priority;

	runPt->status = 2;
	runPt->sleep_cnt = sleepTime;
	
	
	if(pri_num[cur_pri] == 1){ // if this sleeping thread is the only thread with this priority
			
		// find the highest priority active thread to run
		 for(i = 0; i<PRI_MAX; i++){	
			if((pri_num[i] != 0) && (pri_start[i]->status ==1)){ // for this, we have to make sure that pri_start is never sleeping
				next_pri = i;
				break;
			}
		}
	
		runPt_next = pri_start[next_pri];
				
	}
	else{ // if there is a thread with the same priority as sleeping one
		//set runPt_next
		runPt_next = runPt->next;
		
		// reconnect active TCBs
		runPt->prev->next = runPt->next;
		runPt->next->prev = runPt->prev;
		
		if(runPt == pri_start[cur_pri]){ // if the sleeping thread is pri_start
			pri_start[cur_pri] = runPt->next;
		}
		
	}
	

		
	// connect sleeping threads 
	if(sleep_num == 0)
	{
		sleep_start = sleepPt;	// set sleepPt
		
		sleepPt->next = sleepPt;
		sleepPt->prev = sleepPt;
	}
	else
	{
		sleepPt->next = sleep_start;
		sleepPt->prev = sleep_start->prev;
		
		sleepPt->prev->next = sleepPt;
		sleepPt->next->prev = sleepPt;
	}
	

	// update cnts
	OS_Wait(&sActive_num);
	pri_num[cur_pri]--;
	OS_Signal(&sActive_num);
	
	OS_Wait(&sSleep_num);
	sleep_num++;
	OS_Signal(&sSleep_num);
	

	
#endif
	
#if check_timestamp
	if(!timestamp_done){
		TimeMessage[time_index]= "OS_Sleep: ";
		TimeStamp[time_index++] = (0xFFFFFFFF-OS_Time())/80000;
		if (time_index==100)  timestamp_done = 1;
	}
#endif
	NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV, and here, runPt is changed
	
#if check_timestamp
	stop_time	= OS_MsTime();	// timing purpose
	no_interrupt_time += OS_TimeDifference(start_time, stop_time);
#endif
	EndCritical(status);

}


// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value)
{
	semaPt->Value = value;
}


// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt) 
{
	uint32_t status = StartCritical();
	semaPt->Value--;
	if(semaPt->Value < 0) {
	runPt->status = 4; // blocked status
	runPt->block_sema = semaPt;

	blocked_pri_num[runPt->priority]++;
	EndCritical(status);

	OS_Suspend();
	}
	EndCritical(status);
}

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt) 
{
	uint32_t status = StartCritical();
	
	semaPt->Value++;
//	int wake_pri; 
	
	if(semaPt->Value <= 0)
	{ // wake up one thread: search by block_sema == semaPt => block_sema = null
		
#if Lab2
		TCB* pt = runPt;
		while(pt->block_sema != semaPt)
		{
			pt = pt->next;
		}
		if(pt->block_sema == semaPt) pt->block_sema = NULL; // waking up
#endif
		
#if Lab3
		for(int i = 0; i < PRI_MAX; i++)
		{
			if(blocked_pri_num[i] > 0){
				TCB* pt = pri_start[i];
				while(pt->block_sema != semaPt)
				{
					pt = pt->next;
					if(pt == pri_start[i] || pt->block_sema == semaPt) break;
				}
		
				if(pt->block_sema == semaPt)
				{
					pt->block_sema = NULL; // waking up
					pt->status = 1;
					
					blocked_pri_num[i]--;
					break;
				}
			}			
		}
#endif
	}
	EndCritical(status);
}


// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt)
{
	DisableInterrupts();
	while(semaPt->Value == 0)
	{
		EnableInterrupts();
		DisableInterrupts();
	}
	semaPt->Value = 0;
	EnableInterrupts();
}

// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt)
{
	uint32_t status = StartCritical();
	semaPt->Value = 1;
	
	EndCritical(status);
}	





//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void)
{
	return runPt->id;
}


void GPIOPortF_Handler(void)
{
	int portf = GPIO_PORTF_MIS_R;
	
	if(sw1_priority <= sw2_priority)	// sw1 has higher priority
	{		
		if(portf & 0x10) // switch 1 pressed
		{
			GPIO_PORTF_ICR_R |= 0x10;      // acknowledge flag4
			if(sw1task != NULL)
				sw1task();
		}
		if(portf & 0x01) // switch 2 pressed
		{
			GPIO_PORTF_ICR_R |= 0x01;      // acknowledge flag0
			if(sw2task != NULL)
				sw2task();
		}
	}
	
	else
	{
		if(portf & 0x01) // switch 2 pressed
		{
			GPIO_PORTF_ICR_R |= 0x01;      // acknowledge flag0
			if(sw2task != NULL)
				sw2task();
		}
		if(portf & 0x10) // switch 1 pressed
		{
			GPIO_PORTF_ICR_R |= 0x10;      // acknowledge flag4
			if(sw1task != NULL)
				sw1task();
		}
	}
	
	return;
}

//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW1Task(void(*task)(void), unsigned long priority)
{
	sw1task = task;
	sw1_priority = priority;
	return 1;
}

//******** OS_AddSW2Task *************** 
// add a background task to run whenever the SW2 (PF0) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// This task can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddSW2Task(void(*task)(void), unsigned long priority)
{
	sw2task = task;
	sw2_priority = priority;
	return 1;
}

// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void)
{
	int32_t status = StartCritical();
#if check_timestamp
	unsigned long start_time = OS_MsTime(); // time measurements
	unsigned long stop_time;	
#endif
	runPt->status = 3;
	
#if Lab2
	// connect the prev and next	of runPt
	runPt->prev->next = runPt->next;
	runPt->next->prev = runPt->prev;
	
	if(runPt == active_start)
		active_start = runPt->next;
	
	OS_Wait(&sActive_num);
	active_num--;
	OS_Signal(&sActive_num);
#endif
#if Lab3
	
	int i=0;
	int cur_pri = runPt->priority;
	int next_pri;
	
	if(pri_num[cur_pri] == 1){ // if this killed thread was the only thread with this priority
			// find the highest priority active thread to run

		 for(i = 0; i<PRI_MAX; i++){	
			if((pri_num[i] != 0) && (pri_start[i]->status ==1)){ // for this, we have to make sure that pri_start is never sleeping
				next_pri = i;
				break;
			}
		}
	
		runPt_next = pri_start[next_pri];
				
	}
	else{ // if there is a thread with the same priority as killing one
		runPt->prev->next = runPt->next;
		runPt->next->prev = runPt->prev;
		
		if(runPt == pri_start[cur_pri]){ // if the killing thread is pri_start
			pri_start[cur_pri] = runPt->next;
		}
		runPt_next = runPt->next;
	}

	//set runPt_next for PendSV context switch
	//runPt_next = runPt->next;

	
	OS_Wait(&sActive_num);
	(pri_num[cur_pri])--;
	OS_Signal(&sActive_num);
	
#endif	
	
	NVIC_INT_CTRL_R = 0x10000000;    // trigger PendSV, and here, runPt is changed
	
	EndCritical(status);
	
	
}

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size)
{
	 putPt = &Fifo[0];
   getPt = &Fifo[0];
   OS_InitSemaphore(&FIFOcurrentSize, 0);
   OS_InitSemaphore(&FIFOmutex, 1);
	 OS_InitSemaphore(&FIFOroomleft, size);

}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data)
{
 #if Lab2 

      OS_Wait(&FIFOroomleft);   // semaphore for fifo size
      OS_Wait(&FIFOmutex);   // reading fifo mutex
      
      *putPt = data;
     putPt++;
			if(putPt == &Fifo[FIFOSIZE]){
				putPt = &Fifo[0];
			}
			
      OS_Signal(&FIFOmutex);
			OS_Signal(&FIFOcurrentSize);

      return 1;
#endif	 
#if Lab3	 

     OS_Wait(&FIFOroomleft);   // semaphore for fifo size
     OS_Wait(&FIFOmutex);   // reading fifo mutex     
      *putPt = data;
     putPt++;
			if(putPt == &Fifo[FIFOSIZE]){
				putPt = &Fifo[0];
			}
			
      OS_Signal(&FIFOmutex);
			OS_Signal(&FIFOcurrentSize);

      return 1;
#endif
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void)
{
#if Lab2	 	 

   int ret;
   
	 OS_Wait(&FIFOcurrentSize);
   OS_Wait(&FIFOmutex);
   
   ret = *getPt;
	 getPt++;
	 if(getPt == &Fifo[FIFOSIZE]){
		 getPt = &Fifo[0];
	 }
	 
   OS_Signal(&FIFOmutex);
   OS_Signal(&FIFOroomleft);
	
   return ret;
	 
#endif	 
#if Lab3	 	 
	   int ret;
 	 OS_Wait(&FIFOcurrentSize);
   OS_Wait(&FIFOmutex);  
   
   ret = *getPt;
	 getPt++;
	 if(getPt == &Fifo[FIFOSIZE]){
		 getPt = &Fifo[0];
	 }
	 
	 OS_Signal(&FIFOmutex);
   OS_Signal(&FIFOroomleft);
	 

   return ret; 
#endif	 	 
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void)
{
   return FIFOcurrentSize.Value;
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void)
{
   OS_InitSemaphore(&Send, 1);
   OS_InitSemaphore(&Ack, 0);
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data)
{

	OS_Wait(&Send);
	Mail = data;
	OS_Signal(&Ack);	

}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void)
{
	unsigned long data;

	OS_Wait(&Ack);
	data=Mail;
	OS_Signal(&Send);

	return data;
	
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295 
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void)
{

	return TIMER0_TAR_R; 
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop)
{//start = last_time, stop = start_time, start> stop usually
  if(stop < start){
    return (start - stop);
  }
	else{
		return (OS_TIME_PERIOD - start + stop);
	}
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void)
{
	ms = 0;
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void)
{
	return ms;
}

/********* Timer0_Handler **********/     
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER0 timeout
//  (*PeriodicTask1)();                // execute user task
	toggle ^= 1;
	ms += toggle;
}




unsigned long project_OS_TimeDifference(unsigned long start, unsigned long stop)
{//start = last_time, stop = start_time, start> stop usually
  if(stop < start){
    return (start - stop);
  }
	else{
		return (OS_TIME_PERIOD - start + stop);
	}
}

