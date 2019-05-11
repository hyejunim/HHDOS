#ifndef TCB_H
#define TCB_H

#include <stdint.h>

#define STACK_SIZE   256      // number of 32-bit words in stackTCB* thread

struct  Sema4{
  long Value;   // >0 means free, otherwise means busy        
// add other components here, if necessary to implement blocking
};
typedef struct Sema4 Sema4Type;


typedef struct tcb
{
	uint32_t* sp;							// stack pointer
	struct tcb* next;				// next tcb pointer
	struct tcb* prev;				// previous tcb pointer
	unsigned long id;			// thread id
	int16_t sleep_cnt;		// sleep counter
	uint16_t age; 						// how long this thread has been active
	uint8_t priority;					// priority
	uint8_t status; 					// status 0: Run, 1: Active, 2: Sleep, 3: Dead, 4: Blocked(not on the slide)
	uint32_t  stack[STACK_SIZE];
	Sema4Type* block_sema;	
	uint32_t pid;
}TCB;


typedef struct pcb
{
	uint32_t pid;					// Process ID
	uint32_t tnum;				// number of threads
	uint32_t* dataPt;		// pointer to data segment
	uint32_t* codePt;	// pointer to code segment
}PCB;

#endif
