/*!
 * @file  VL53L0X_Distance_Measure_1_ST7735.c
 * @brief Use VL53L0X to measure distance and display it to ST7735 LCD screen.
 * ----------
 * Adapted code from Adafruit VL53L0X driver for Arduino.
 * You can find the Adafruit VL53L0X driver here: https://github.com/adafruit/Adafruit_VL53L0X
 * ----------
 * ST VL53L0X datasheet: https://www.st.com/resource/en/datasheet/vl53l0x.pdf
 * ----------
 * For future development and updates, please follow this repository: https://github.com/ZeeLivermorium/VL53L0X_TM4C123
 * ----------
 * If you find any bug or problem, please create new issue or a pull request with a fix in the repository.
 * Or you can simply email me about the problem or bug at zeelivermorium@gmail.com
 * Much Appreciated!
 * ----------
 * @author Zee Livermorium
 * @date   Aug 5, 2018
 */

#include <stdint.h>
#include <stdlib.h>
//#include "hw_types.h"
//#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "I2C.h"
#include "ST7735.h"
#include "OS.h"
#include "UART2.h"
#include "Interpreter.h"


#define TIMESLICE 1000*TIME_1MS  // thread switch time in system time units

void DisableInterrupts(void); // Disable interrupts

void IdleTask(void);


unsigned long NumCreated;   // number of foreground threads created
int main(void) {	// osmain
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

