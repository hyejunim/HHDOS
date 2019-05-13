#include <stdint.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Thread.h"



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


