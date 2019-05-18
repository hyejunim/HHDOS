;/*****************************************************************************/
;/* OSasm.s: low-level OS commands, written in assembly                       */
;/* derived from uCOS-II                                                      */
;/*****************************************************************************/
;Jonathan Valvano, OS Lab2/3/4 solution, 3/9/17
GPIO_PORTF_DATA_R  EQU 0x400253FC

	AREA globals
	IMPORT runPt
	IMPORT runPt_next
	
        AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXTERN  RunPt            ; currently running thread
        EXTERN  NextThreadPt     ; next thread to run, set by schedule
        EXPORT  StartOS
        EXPORT  ContextSwitch
        EXPORT  PendSV_Handler
				
		IMPORT	OS_Id
        IMPORT  	OS_Sleep
		IMPORT	OS_Kill
		IMPORT	OS_Time
		IMPORT	OS_AddThread


NVIC_INT_CTRL   EQU     0xE000ED04                              ; Interrupt control state register.
NVIC_SYSPRI14   EQU     0xE000ED22                              ; PendSV priority register (position 14).
NVIC_SYSPRI15   EQU     0xE000ED23                              ; Systick priority register (position 15).
NVIC_LEVEL14    EQU           0xEF                              ; Systick priority value (second lowest).
NVIC_LEVEL15    EQU           0xFF                              ; PendSV priority value (lowest).
NVIC_PENDSVSET  EQU     0x10000000                              ; Value to trigger PendSV exception.



	EXPORT StartOS
StartOS
	LDR	R0, =runPt	;currently running thread
	LDR	R1, [R0]		; R1 = value of runPt
	LDR	SP, [R1]		;new thread SP, SP = runPt->sp;
	POP	{R4-R11}		; restore registers R4~R11
	POP	{R0-R3}		; restore registers R0~R3
	POP	{R12}			; restore registers R12
	ADD	SP, SP, #4	; discard LR from initial stack
	POP {LR}			; start location
	ADD	SP, SP, #4	; discard PSR from initial stack
	CPSIE	I				; Enable interrupts at processor level
	BX 	LR				; start first thread



OSStartHang
    B       OSStartHang        ; Should never get here


;********************************************************************************************************
;                               PERFORM A CONTEXT SWITCH (From task level)
;                                           void ContextSwitch(void)
;
; Note(s) : 1) ContextSwitch() is called when OS wants to perform a task context switch.  This function
;              triggers the PendSV exception which is where the real work is done.
;********************************************************************************************************

ContextSwitch
    LDR     R0, =NVIC_INT_CTRL        ; Trigger the PendSV exception (causes context switch)
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]
    BX      LR
    

;********************************************************************************************************
;                                         HANDLE PendSV EXCEPTION
;                                     void OS_CPU_PendSVHandler(void)
;
; Note(s) : 1) PendSV is used to cause a context switch.  This is a recommended method for performing
;              context switches with Cortex-M3.  This is because the Cortex-M3 auto-saves half of the
;              processor context on any exception, and restores same on return from exception.  So only
;              saving of R4-R11 is required and fixing up the stack pointers.  Using the PendSV exception
;              this way means that context saving and restoring is identical whether it is initiated from
;              a thread or occurs due to an interrupt or exception.
;
;           2) Pseudo-code is:
;              a) Get the process SP, if 0 then skip (goto d) the saving part (first context switch);
;              b) Save remaining regs r4-r11 on process stack;
;              c) Save the process SP in its TCB, OSTCBCur->OSTCBStkPtr = SP;
;              d) Call OSTaskSwHook();
;              e) Get current high priority, OSPrioCur = OSPrioHighRdy;
;              f) Get current ready thread TCB, OSTCBCur = OSTCBHighRdy;
;              g) Get new process SP from TCB, SP = OSTCBHighRdy->OSTCBStkPtr;
;              h) Restore R4-R11 from new process stack;
;              i) Perform exception return which will restore remaining context.
;
;           3) On entry into PendSV handler:
;              a) The following have been saved on the process stack (by processor):
;                 xPSR, PC, LR, R12, R0-R3
;              b) Processor mode is switched to Handler mode (from Thread mode)
;              c) Stack is Main stack (switched from Process stack)
;              d) OSTCBCur      points to the OS_TCB of the task to suspend
;                 OSTCBHighRdy  points to the OS_TCB of the task to resume
;
;           4) Since PendSV is set to lowest priority in the system (by OSStartHighRdy() above), we
;              know that it will only be run when no other exception or interrupt is active, and
;              therefore safe to assume that context being switched out was using the process stack (PSP).
;********************************************************************************************************
	
	EXPORT PendSV_Handler
PendSV_Handler
	CPSID   I                  ; Prevent interruption during context switch
    PUSH    {R4-R11}           ; Save remaining regs r0-11 
    LDR     R0, =runPt         ; R0=pointer to RunPt, old thread
    LDR     R1, [R0]         ; RunPt->stackPointer = SP;
    STR     SP, [R1]           ; save SP of process being switched out
	
	LDR		R3, =runPt_next ; R3 is address of runPt_next
	LDR		R2, [R3]			 ; R2 = next Thread address
    STR     R2, [R0]			; save R2 value to runPt

	LDR		R1, [R2,#4]		; R2 = next Thread address
	STR		R1, [R3]		;update runPt_next value

    LDR     SP, [R2]           ; new thread SP; SP = RunPt->stackPointer;
    POP     {R4-R11}           ; restore regs r4-11 

    CPSIE   I               ; tasks run with I=0
    BX      LR                 ; Exception return will restore remaining context   


;******************************************************************************
; normal OS_Wait and OS_Signal 
;******************************************************************************
	;EXPORT OS_Wait
;OS_Wait
	;LDREX	R1, [R0] 				; counter. assume that the address of semaphore is in R1 initially
	;SUBS	R1, #1					; counter -1
	;ITT		PL						; next two instructions are conditional, ok if  >=0
	;STREXPL	R2, R1, [R0]	; try update 
	;CMPPL	R2, #0					; succeed? (If R2 is 0, load-store successful)
	;BNE		OS_Wait				; no, try again
	;BX		LR


	;EXPORT OS_Signal
;OS_Signal
	;LDREX	R1, [R0] 				; counter. assume that the address of semaphore is in R1 initially
	;ADD		R1, #1					; counter +1
	;STREX	R2, R1, [R0]		; try update
	;CMP		R2, #0					; succeed?
	;BNE		OS_Signal			; no, try again
	;BX		LR
	
	EXPORT SVC_Handler
SVC_Handler 
;SVC_Handler automatically push and pop
;		R0-R3, R12, LR, PSR, return address
; OS_Id #0, OS_Kill #1, OS_Sleep #2, OS_Time #3, OS_AddThread #4

	LDR 		R12, [SP, #24]			; return address
	LDRH	R12, [R12, #-2]		; SVC instruction is 2 bytes (LDRH: H means, unsigned halfword, zero extend to 32 bits on loads)
	BIC 		R12, #0xFF00			; Extract ID in R12 (BIC: bit clear)
	LDM 	SP, {R0-R3}			; Get any parameters (LDM: load multiple registers, SP is the base register, R0-R3 is loaded)

;	MOV 	R12, #1
	PUSH	{LR}
	CMP		R12, #0					; OS_Id
	BLEQ	OS_Id
	CMP		R12, #1					; OS_Kill
	BLEQ	OS_Kill
	CMP		R12, #2					; OS_Sleep
	BLEQ	OS_Sleep
	CMP		R12, #3					; OS_Time
	BLEQ	OS_Time
	CMP		R12, #4					; OS_AddThread
	BLEQ	OS_AddThread
		
	POP		{LR}
	STR		R0, [SP]					; store return value
	BX		LR							; return from exception



    ALIGN
    END
