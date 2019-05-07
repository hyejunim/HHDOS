/******** Master_Main ********
* Author: Hyejun Im (hi956), Hyunsu Chae (hc25673)
*/

#include <stdint.h>
#include <stdlib.h>
#include "PLL.h"
#include "I2C.h"
#include "ST7735.h"
#include "OS.h"
#include "UART2.h"
#include "Interpreter.h"
#include "master_ssi.h"


#define TIMESLICE 1000*TIME_1MS  // thread switch time in system time units

void EnableInterrupts(void); // Enable interrupts
void DisableInterrupts(void); // Disable interrupts

void IdleTask(void);



/************************
*  SSI Communication test *
************************/
int main()
{
	DisableInterrupts(); // Disable all interrupts
	
    /*-- TM4C123 Init --*/
    PLL_Init(Bus80MHz);                             // bus clock at 80 MHz
    
    /*-- ST7735 Init --*/
    ST7735_InitR(INITR_REDTAB);
    ST7735_FillScreen(ST7735_BLACK);
	
	while(1) {
		
	}
}










/************************
*             OS MAIN             *
************************/
unsigned long NumCreated;   // number of foreground threads created
int osmain(void) {	// osmain
		DisableInterrupts(); // Disable all interrupts
	
    /*-- TM4C123 Init --*/
    PLL_Init(Bus80MHz);                             // bus clock at 80 MHz
    
		OS_Init();           // initialize
	
    /*-- ST7735 Init --*/
    ST7735_InitR(INITR_REDTAB);
    ST7735_FillScreen(ST7735_BLACK);

	
		NumCreated = 0 ;
    
		// create initial foreground threads
		NumCreated += OS_AddThread(Interpreter,128,2); // priority 2
		NumCreated += OS_AddThread(&IdleTask,128,7);  // priority 7 runs when nothing useful to do

		OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
		return 0;             // this never executes
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

