// *************Interpreter.c**************
// EE445M/EE380L.6 Lab 4 solution
// high level OS functions
// 
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 3/9/17, valvano@mail.utexas.edu
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ST7735.h"
#include "OS.h"
#include "UART2.h"


#define INPUT_SIZE 20

Sema4Type sUART;

void interpreter_lcd(void);
void interpreter_ir(void);
void interpreter_laser(void);
void interpreter_help(void);

char input[INPUT_SIZE] = {0,};
char *tok[10] = {0,};

void Interpreter_Init(void)
{
	OS_InitSemaphore(&sUART, 1);
}

//---------------------OutCRLF---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
void UART_OutCRLF(void){
	OS_bWait(&sUART);
  UART_OutChar(CR);
  UART_OutChar(LF);
	OS_bSignal(&sUART);
}

void Interpreter(void)
{
	while(1) {
		UART_OutCRLF();	UART_OutString("HH: ");
		UART_InString(input, INPUT_SIZE);
		
		int i = 0;
		// tokenize input string
		tok[i] = strtok(input, " -");
		while (tok[i] != NULL)
			tok[++i] = strtok(NULL, " -");
		
		
		//OS_Wait(&sUART);
		// determine commands
		if(!strcmp(tok[0], "lcd"))								interpreter_lcd();
		
		else if(!strcmp(tok[0], "help"))		interpreter_help();
		else { UART_OutCRLF(); UART_OutString("command not found, type 'help'"); }
		
		//OS_Signal(&sUART);
	}
}

void interpreter_help(void)
{
	UART_OutCRLF(); UART_OutString("lcd   < <device> <line> <string> <value> >");
	UART_OutCRLF(); UART_OutString("ir   	< <device> <v, c> >");
	UART_OutCRLF(); UART_OutString("laser <device>");
}



void interpreter_lcd(void)
{
	int device, line;
	int32_t value;
	
	device = atoi(tok[1]);
	line = atoi(tok[2]);
	value = atoi(tok[4]);
	
	if(!(device == 0  || device == 1))
	{
		UART_OutCRLF(); UART_OutString("Wrong device input. Value should be 0 or 1.");
		return;
	}
	else if(!(0 <= line && line <= 7)) 
	{
		UART_OutCRLF(); UART_OutString("Wrong line input. Value should be less than 8.");
		return;
	}
	
	ST7735_Message(device, line, tok[3], value);
}






