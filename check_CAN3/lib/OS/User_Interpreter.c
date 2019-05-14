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
#include "User_Interpreter.h"
#include "can0.h"
#include "user.h"

#define INPUT_SIZE 20

Sema4Type sUART;

void interpreter_lcd(void);
void interpreter_help(void);

void interpreter_filels(void);
void interpreter_fileread(void);
void interpreter_filewrite(void);
void interpreter_filerm(void);
void interpreter_fileformat(void);

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
		if(!strcmp(tok[0], "lcd"))							interpreter_lcd();
		else if(!strcmp(tok[0], "ls"))						interpreter_filels();
		else if(!strcmp(tok[0], "cat"))					interpreter_fileread();
		else if(!strcmp(tok[0], "more"))				interpreter_fileread();
		else if(!strcmp(tok[0], "vi"))						interpreter_filewrite();
		else if(!strcmp(tok[0], "rm"))					interpreter_filerm();
		else if(!strcmp(tok[0], "disk_format"))		interpreter_fileformat();
		
		else if(!strcmp(tok[0], "help"))		interpreter_help();
		else { UART_OutCRLF(); UART_OutString("command not found, type 'help'"); }
		
		//OS_Signal(&sUART);
	}
}

void interpreter_filels(void)
{
	uint8_t request[8], response[8], file[8];
	uint32_t filenum = 0;
	
	// 1) send request to the server
	request[0] = LIST;
	CAN0_SendData(request);
	
	// 2) wait response from the server
	CAN0_GetMail(response);
	if(response[0] == '*')	// approved
	{
		for(int i=7; i>3; i--)
			filenum += response[i] << ((7-i)*8);
		filenum >>= 3;
	}
	else								// denied
	{
		UART_OutCRLF();
		for(int j=0; j<8; j++)	UART_OutChar(response[j]);
		return;
	}
	
	// 3) output on the interpreter
	 UART_OutCRLF(); 
	for(int i=0; i<filenum; i++)
	{
		CAN0_GetMail(file);
		for(int j=0; j<8; j++)	UART_OutChar(file[j]);	UART_OutString("   ");
	}
}

void interpreter_fileread(void)
{
	uint8_t request[8], response[8], file[8];
	uint32_t filesize = 0;
	
	if(strlen(tok[1]) > 7) return;
	
	// 1) send request to the server
	request[0] = READ;
	for(int i=1; i<8; i++)
		request[i] = tok[1][i-1];
	CAN0_SendData(request);
	
	// 2) wait request from the server
	CAN0_GetMail(response);
	if(response[0] == '*')	// approved
	{
		for(int i=7; i>3; i--)
			filesize += response[i] << ((7-i)*8);
	}
	else								// denied
	{
		 UART_OutCRLF(); 
		for(int j=0; j<8; j++)	UART_OutChar(response[j]);	
		return;
	}
	
	// 3) output on the interpreter
	 UART_OutCRLF(); 
	for(int i=0; i<filesize; i+=8)
	{
		CAN0_GetMail(file);
		for(int j=0; j<8; j++)	UART_OutChar(file[j]);
		//UART_OutString((char*)file);
	}
}

void FSM_Save(char in, uint8_t* state_save)
{
	switch(*state_save)	// FSM for :wq
	{
		case 0: 
			if(in == ESC) *state_save = 1;
			else *state_save = 0;
		break;
		case 1: 
			if(in == ':') *state_save = 2;
			else *state_save = 0;
		break;
		case 2: 
			if(in == 'w') *state_save = 3;
			else *state_save = 0;
		break;
		case 3: 
			if(in == 'q') *state_save = 4;
			else *state_save = 0;
		break;
		default:
			break;
	}
}
void FSM_Quit(char in, uint8_t* state_quit)
{
	switch(*state_quit)	// FSM for :q!
	{
		case 0: 
			if(in == ESC) *state_quit = 1;
			else *state_quit = 0;
		break;
		case 1: 
			if(in == ':') *state_quit = 2;
			else *state_quit = 0;
		break;
		case 2: 
			if(in == 'q') *state_quit = 3;
			else *state_quit = 0;
		break;
		case 3: 
			if(in == '!') *state_quit = 4;
			else *state_quit = 0;
		break;
		default:
			break;
	}
}

uint8_t wqfilename[7] = {0,};
uint8_t wqoriginal[1024] = {0,};
uint8_t wqmodified[1024] = {0,};
uint16_t wqfilesize = 0;

void modify(void)
{
	uint8_t state_save = 0;
	uint8_t state_quit = 0;
	while(1)
	{
		char in = UART_InChar();
		FSM_Save(in, &state_save);
		FSM_Quit(in, &state_quit);
		if(state_save == 4) // save and quit
		{
			flag_wqsend = 1;
			return;
		}
		else if(state_quit == 4)
		{
			flag_qsend = 1;
			return;
		}
		else if(state_save == 0 && state_quit == 0)	// 
		{
			// modify modified array
			if(in == DEL) // backspace
				wqfilesize--;
			else
				wqmodified[wqfilesize] = in;
			
			// output on the screen
			UART_OutChar(in);	
		}
	}
}
void interpreter_filewrite(void)
{
	uint8_t request[8], response[8], file[8];
	uint32_t filesize = 0;
	wqfilesize = 0;
	
	if(strlen(tok[1]) > 7) return;
	
	// 1) send request to the server
	request[0] = WRITE;
	for(int i=1; i<8; i++) {
		request[i] = tok[1][i-1];
		wqfilename[i] = tok[1][i-1];
	}
	CAN0_SendData(request);
	
	// 2) wait request from the server
	CAN0_GetMail(response);
	if(response[0] == '*')	// approved
	{
		for(int i=7; i>3; i--)
			filesize += response[i] << ((7-i)*8);
	}
	else								// denied
	{
		UART_OutCRLF();
		for(int j=0; j<8; j++)	UART_OutChar(response[j]);	
		return;
	}
	
	// 3) output on the interpreter
	UART_OutCRLF(); 
	for(int i=0; i<filesize; i+=8)
	{
		CAN0_GetMail(file);
		for(int j=0; j<8; j++)
		{
			UART_OutChar(file[j]);	
			wqoriginal[wqfilesize] = file[j];
			wqmodified[wqfilesize] = file[j];
			wqfilesize++;
		}
	}
	
	// 4) modify the file (only backspace and concat)
	modify();
}

void interpreter_filerm(void)
{
	uint8_t request[8], response[8];
	
	if(strlen(tok[1]) > 7) return;
	
	// 1) send request to the server
	request[0] = REMOVE;
	for(int i=1; i<8; i++)
		request[i] = tok[1][i-1];
	CAN0_SendData(request);
	
	// 2) wait response from the server
	CAN0_GetMail(response);
//	if(response[0] == '*')	// approved
//	{}
//	else								// denied
//	{
		 UART_OutCRLF();
		for(int j=0; j<8; j++)	UART_OutChar(response[j]);	
//		return;
//	}
}

void interpreter_fileformat(void)
{
	uint8_t request[8], response[8];
	
	// 1) send request to the server
	request[0] = FORMAT;
	CAN0_SendData(request);
	
	// 2) wait response from the server
	CAN0_GetMail(response);
//	if(response[0] == '*')	// approved
//	{}
//	else								// denied
//	{
		 UART_OutCRLF();
		for(int j=0; j<8; j++)	UART_OutChar(response[j]);	
//		return;
//	}
}
void interpreter_help(void)
{
	UART_OutCRLF(); UART_OutString("lcd   < <device> <line> <string> <value> >");
	UART_OutCRLF(); UART_OutString("0: ls   	");
	UART_OutCRLF(); UART_OutString("1: cat 	<filename>");
	UART_OutCRLF(); UART_OutString("1: more 	<filename>");
	UART_OutCRLF(); UART_OutString("2: vi 	<filename>");
	UART_OutCRLF(); UART_OutString("3: rm 	<filename>");
	UART_OutCRLF(); UART_OutString("4: disk_format         ");
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






