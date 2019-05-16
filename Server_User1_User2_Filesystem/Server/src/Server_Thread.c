#include <stdint.h>
#include <string.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Server_Thread.h"

#include "Interpreter.h"
#include "UART2.h"

#include "eFile.h"

#define RECORD_NUM 100


#define MAX_FILESIZE 100
#define MAX_FILENUM 10

void printc(uint8_t data[8]){
	for(int i=0; i<8; i++){
		UART_OutChar(data[i]); UART_OutString(" ");
	}
	UART_OutCRLF();
}
void printd(uint8_t data[8]){
	for(int i=0; i<8; i++){
		UART_OutUDec(data[i]); UART_OutString(" ");
	}
	UART_OutCRLF();
}

///////////////////////////////////////*******RECORD and DIRECTORY*******///////////////////////////////////////////////////////

//******** struct for record  *************** 
typedef struct Table
{
	char file_name[8];
	uint8_t user; // who is in charge of this file: 0-server, 1-user1, 2-user2
	
} RECORD;

RECORD record[RECORD_NUM];


//******** Record_Init  *************** 
void Record_Init(void){
	
	for(int i=0; i<RECORD_NUM; i++){
		record[i].file_name[0] = 0;
	}
}



//******** Check_Record ***************
// check record and return the record_index if the file exist in the record
// 															return 0 if the file does not exist
// record[0] is empty....... ALWAYS
int user1_r_index = 0;
int user2_r_index = 0;
void Check_Record(char * name, uint8_t cur_user){
	int i;

	if( cur_user == 1){
		for(i=1; i< RECORD_NUM; i++){
			if(strcmp(record[i].file_name, name) == 0){ // found the file in the record
				if( record[i].user == 2){ 												// different from what is on the record -> denied
							user1_r_index = 100;
							break;
				}
				else{																												// same from what is on the record -> can take it
						user1_r_index=i;
						break;
				}
			}
		}
		if(i == RECORD_NUM){ // not in record -> accepted
			user1_r_index = 0;
		}
	}
	if( cur_user == 2){
			for(i=1; i< RECORD_NUM; i++){
				if(strcmp(record[i].file_name, name) == 0){ // found the file in the record
					if( record[i].user == 1){ 												// different from what is on the record -> denied
								user2_r_index = 100;
								break;
					}
					else{																												// same from what is on the record -> can take it
							user2_r_index=i;
							break;
					}
				}
			}
			if(i == RECORD_NUM){ // not in record -> accepted
				user2_r_index = 0;
			}
	}
	
	
}

//******** Check_Directory ***************
// check directory and update the user1_d_index if the file exist in the directory
// 																update the user1_d_index to 0  if the file  does not exist
// direcotory[0] is empty.......

int user1_d_index = 0;
int user2_d_index = 0;
void Check_Directory(char * name, uint8_t user){
	
	int i;
	if(user == 1){
			for(i=1; i< DIR_SIZE; i++){ 
				if(strcmp(directory[i].file_name, name) == 0){ // found the file in the directory 
					user1_d_index=i;
					break;
				}
			}
			// not in directory
			 if(i == DIR_SIZE){ 
				user1_d_index = 0;
			}
	}
	else if(user == 2){
				for(i=1; i< DIR_SIZE; i++){ 
				if(strcmp(directory[i].file_name, name) == 0){ // found the file in the directory 
					user2_d_index=i;
					break;
				}
			}
			// not in directory
			 if(i == DIR_SIZE){ 
				user2_d_index = 0;
			}
		
	}
	
}

//******** ls_Task ***************

uint32_t ls_total_file_num1;
uint8_t ls_filenames1[MAX_FILENUM][8]={0,};
uint32_t ls_total_file_num2;
uint8_t ls_filenames2[MAX_FILENUM][8]={0,};
void ls_Task(uint8_t user){
if(user ==1 ){
	
	ls_total_file_num1 = 0;
	
	for(int i=1; i< DIR_SIZE; i++){ 
		if(directory[i].file_name[0] != NULL){ 
			for(int j=0; j<8; j++){
				ls_filenames1[ls_total_file_num1][j] = directory[i].file_name[j];
			}
			ls_total_file_num1 ++;
		}
	}	
}
if(user ==2 ){
	
	ls_total_file_num2 = 0;
	
	for(int i=1; i< DIR_SIZE; i++){ 
		if(directory[i].file_name[0] != NULL){ 
			for(int j=0; j<8; j++){
				ls_filenames2[ls_total_file_num2][j] = directory[i].file_name[j];
			}
			ls_total_file_num2 ++;
		}
	}	
}
}


//******** Thread_Initialize  *************** 
void Thread_Initialize(void){
	

  if(eFile_Init()){
		UART_OutString("eFile_Init Failed"); UART_OutCRLF();
	}

	  if(eFile_Format()){
		UART_OutString("eFile_Format Failed"); 	UART_OutCRLF();
	}
	
	NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2); //  calls thread that sends 14byte data, priority 1 and kills
	NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2); //  calls thread that sends 14byte data, priority 1 and kills
	
	OS_Kill();
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////*******General Task for Server 1*******////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


uint8_t Rcv1Data[8];
uint8_t Xmt1Data[8];
uint8_t Xmt1_ready=0;
//******** Thread_Rcv1CAN  *************** 
// receiving data sent by USER1
void Thread_Rcv1CAN(void)
{
  uint8_t data[8];
	Xmt1_ready=0;
	
	for(int i=0; i<8; i++) Rcv1Data[i] = 0;
	
	
	UART_OutCRLF(); UART_OutCRLF(); UART_OutString("////****START new COMMAND User1****///"); UART_OutCRLF();
	
		CAN0_GetMail1(data);
		for(int i=0; i<8; i++) 	Rcv1Data[i]=data[i];
		
		UART_OutString(" User1 COMMAND: "); 
		switch (Rcv1Data[0]){
			case 0: UART_OutString("ls  ");				break;
			case 1: UART_OutString("cat/more  ");				break;
			case 2: UART_OutString("vi  ");				break;		
			case 3: UART_OutString("rm  ");				break;		
			case 4: UART_OutString("disk_format  "); break;		
			case 5: UART_OutString(":wq  ");			break;		
			default:  UART_OutUDec(Rcv1Data[0]);
		}
		for(int i=1; i<8; i++){
			UART_OutChar((char)Rcv1Data[i]);
		} UART_OutCRLF();

		
		Server_Rcv1Data_Task();		
		Server_Xmt1Data_Task();
		
		OS_Kill();
}

	char filename1[8];
//********  Server_RcvData_Task *************** 
// does a task regarding data received from users
// check RECORD, and prepare
void Server_Rcv1Data_Task(void)
{
	int temp=0;
	
// 0) re-organize file_name received		
	for(int i=0; i<7; i++) filename1[i]=NULL;
	for(int i=0; i<7; i++){
		if(Rcv1Data[i+1] == NULL) break;
		filename1[i] =  Rcv1Data[i+1];	
	} filename1[7] = NULL;
	
	for(int i=0; i<8; i++) Xmt1Data[i] = 0;
	
//1) find record_index and directory_index
	Check_Record(filename1, 1);
	Check_Directory(filename1,1); // update user1_d_index, whichi is global variable
	
//2) do jobs depending on the command
	switch (Rcv1Data[0]){
		case 0: // ls 								
				
				ls_Task(1);
				temp = ls_total_file_num1 * 8;
				Xmt1Data[0] = '*';
				Xmt1Data[7] = temp & 0x000000FF;
				Xmt1Data[6] = (temp & 0x0000FF00)>>8;
				Xmt1Data[5] = (temp & 0x00FF0000)>>16;
				Xmt1Data[4] = (temp & 0xFF000000)>>24;		
		
				break;

		case 1: // cat/more 
				//a) found in the directory -> send file size 
				if(user1_d_index != 0){
					Xmt1Data[0] = '*';
					Xmt1Data[7] = directory[user1_d_index].size & 0x000000FF;
					Xmt1Data[6] = (directory[user1_d_index].size & 0x0000FF00)>>8;
					Xmt1Data[5] = (directory[user1_d_index].size & 0x00FF0000)>>16;
					Xmt1Data[4] = (directory[user1_d_index].size & 0xFF000000)>>24;
				}
				//b) not found in the directory -> "no file"
				else{
					strcpy((char *)Xmt1Data, "no file");
				}
				break;
		
		case 2: // vi
				//a) found in record
				if(user1_r_index ==100){ //  someone else has it
					strcpy((char *)Xmt1Data, "denied");
				}
				//b-1)  not found in record  && found in directory -> update record
				else{
				//b-2)  not found in record  && not found in directory -> make new file with size 0 (update directory) , update record
					if(user1_d_index == 0){
						  if(eFile_Create(filename1)){
								UART_OutString("eFile_Create Failed"); UART_OutCRLF();
							}
							Check_Directory(filename1,1);	// user1_d_index  is not zero anymore
					}
					// send the size of the file
					Xmt1Data[0] = '*';
					Xmt1Data[7] = directory[user1_d_index].size & 0x000000FF;
					Xmt1Data[6] = (directory[user1_d_index].size & 0x0000FF00)>>8;
					Xmt1Data[5] = (directory[user1_d_index].size & 0x00FF0000)>>16;
					Xmt1Data[4] = (directory[user1_d_index].size & 0xFF000000)>>24;		
					
					// update record
					for(int i=1; i<RECORD_NUM; i++){
						if(record[i].file_name[0] == NULL){
							user1_r_index = i;
							break;
						}
					}
					for(int i=0; i<8; i++) record[user1_r_index].file_name[i] = filename1[i];
					record[user1_r_index].user = 1;
				}
				break;
		
		case 3: // rm   
				//a) not found in directory -> "no file"
				if(user1_d_index ==0){
					strcpy((char *)Xmt1Data, "no file");
				}
				//b-1)  found in directory  && found in record -> "denied"
				else{
					if(user1_r_index !=0){
						strcpy((char *)Xmt1Data, "denied");					
					}
				//b-2)  found in directory  && not found in record -> erase the file, "removed"
					else{ 
						strcpy((char *)Xmt1Data, "removed");		
						if(eFile_Delete(filename1)) {
							UART_OutString("eFile_Delete Failed"); UART_OutCRLF();
						}
					}
				}
				break;
			
		case 4: // disk_format
				strcpy((char *)Xmt1Data, "denied");
				break;
		
		case 5: // wq
				//a) not found in directory -> "no file"
//				if(user1_d_index ==0){
//					strcpy((char *)Xmt1Data, "no file");
//				}
				//b-1)  found in directory  && found in record -> "saved", and start receiving, update the record
//				else{
					if(user1_r_index ==100){ // someone else has it
						strcpy((char *)Xmt1Data, "denied");
					}
					else if(user1_r_index !=0 ){ // I have it 
						strcpy((char *)Xmt1Data, "saved");
						//update record
						for(int i=0; i<8; i++) record[user1_r_index].file_name[i] =NULL;
						record[user1_r_index].user = 0;						
					}	
				//b-2)  found in directory  && not found in record -> erase the file, "wrong"
					else{
						strcpy((char *)Xmt1Data, "wrong");		
					}
//				}
				break;
			 
		default: 
				strcpy((char *)Xmt1Data, "weird");
	}
}


//******** Server_Xmt1Data_Task  *************** 
//prepare for sending data: cat/more, vi (0, 1, 2) 
//prepare for receiving data: wq (5)								
//or get another command

int Xmt1_num=0;
int Xmt1_done=0;

void Server_Xmt1Data_Task(void){

	Xmt1_num = 0;
	
	CAN0_SendData1(Xmt1Data);

		UART_OutString(" send reponse to USER1: "); UART_OutCRLF();
		UART_OutString("   for ls, cat/more, vi : "); printd(Xmt1Data);
		UART_OutString("   for rm, disk_format, wq: "); printc(Xmt1Data);		
				
	switch (Rcv1Data[0]){
		case 0: // ls   						// prepare to send data
				Xmt1_num = ls_total_file_num1;
				Xmt1_ready=1; 
				break;
		
		case 1: // cat/more 			
				//a) found in the directory -> send file size 
				if(strcmp((char *)Xmt1Data, "no file") == 0){ 
					 if(directory[user1_d_index].size == 0) Xmt1_num =0;
					 NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
					 Xmt1_ready=0; 
				}
				//b) if NOT denied
				else{ // prepare to send data
					Xmt1_num = (directory[user1_d_index].size-1)/8 + 1;
					Xmt1_ready=1; 
				}	
				break;
		
		case 2: // vi
				//a) if denied
				if(strcmp((char *)Xmt1Data, "denied") == 0){ 
					NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
					Xmt1_ready=0; 
				}
				//b) if NOT denied
				else{ // prepare to send data
					if(directory[user1_d_index].size == 0) Xmt1_num =0;
					else Xmt1_num = (directory[user1_d_index].size-1)/8 +1;
					Xmt1_ready=1; 
				}
				break;
	
		case 3: // rm
				NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
				break;
		
		case 4: // disk_format
				NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
				break;
			 
		case 5: // wq
				if(strcmp((char *)Xmt1Data, "saved") == 0){ // prepare to receive data
					Rcv1_WQ_Task(); 
				}
				else{
					NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
				}
				break;
			 
		default: 
			NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////******* Task only for wq*******//////////////////////////////////////////////////////////


uint32_t wq_can_frame1;
uint32_t wq_file_size1;
//********  Rcv1CAN14B_Task (only used for :wq)***************
// receiving the SIZE of the file: wq_new_size
void Rcv1_WQ_Task(void){

  uint8_t data[8];	

	
	CAN0_GetMail1(data);
	
	wq_file_size1 = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7] ; 
	wq_can_frame1 = (wq_file_size1-1)/8 + 1;
	
	UART_OutString("data for User1 wq size : ");	UART_OutUDec(wq_file_size1); UART_OutCRLF();

	// Delete the original file
		if(eFile_Delete(filename1)) {
			UART_OutString("eFile_Delete Failed"); UART_OutCRLF();
		}
	// re-Create the file with same name, but doing this will make the file size and index 0
		if(eFile_Create(filename1)) {
			UART_OutString("eFile_Create Failed"); UART_OutCRLF();
		}	
		
		if(wq_file_size1 == 0) NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
		else NumCreated += OS_AddThread(Thread_Rcv1_WQ, 128, 2);

}


//********  Thread_Rcv1_WQ (only used for :wq)***************
void Thread_Rcv1_WQ(void){
	
	uint8_t new_file[MAX_FILESIZE];	// file data
	for(int i=0; i<MAX_FILESIZE; i++){
		new_file[i]=NULL;
	}
	
	uint8_t data[8];	
	
	
	for(int i=0; i<wq_can_frame1; i++){
		
		for(int j=0; j<8; j++) 	data[j] = NULL;
		
		CAN0_GetMail1(data);
		
		UART_OutString(" RECEIVED from User1  ");  UART_OutUDec(i); UART_OutCRLF();
		printc(data);
		
		for(int j=0; j<8; j++){
			new_file[i*8 + j] = data[j];
		}
	}
	
	
	if(eFile_WOpen(filename1)){
			UART_OutString("eFile_WOpen Failed"); UART_OutCRLF();
	}
  for(int i=0;i<wq_file_size1;i++){   
    if(eFile_Write((char)new_file[i])){
			UART_OutString("eFile_Write Failed"); UART_OutCRLF();
		}
  }
	if(eFile_WClose()){   
			UART_OutString("eFile_WClose Failed"); UART_OutCRLF();
	}
		
	UART_OutString(" User1 new file content:  "); 
	for(int i=0; i<wq_file_size1; i++) UART_OutChar(new_file[i]); 
	UART_OutCRLF();
	
	NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
	OS_Kill();
	
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////*******General Task for Server 2*******//////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


uint8_t Rcv2Data[8];
uint8_t Xmt2Data[8];
uint8_t Xmt2_ready=0;

//******** Thread_Rcv2CAN  *************** 
// receiving data sent by USER2
void Thread_Rcv2CAN(void)
{
  uint8_t data[8];
	Xmt2_ready=0;
	
	for(int i=0; i<8; i++){
		Rcv2Data[i] = 0;
	}
	
	UART_OutCRLF(); UART_OutCRLF(); UART_OutString("////****START new COMMAND User2****///"); UART_OutCRLF();
	
		CAN0_GetMail2(data);
		for(int i=0; i<8; i++) 	Rcv2Data[i]=data[i];
		
		UART_OutString(" COMMAND User2: "); 
		switch (Rcv2Data[0]){
			case 0: UART_OutString("ls  ");				break;
			case 1: UART_OutString("cat/more  ");				break;
			case 2: UART_OutString("vi  ");				break;		
			case 3: UART_OutString("rm  ");				break;		
			case 4: UART_OutString("disk_format  "); break;		
			case 5: UART_OutString(":wq  ");			break;		
			default:  UART_OutUDec(Rcv2Data[0]);
		}
		for(int i=1; i<8; i++){
			UART_OutChar((char)Rcv2Data[i]);
		} UART_OutCRLF();

		
		Server_Rcv2Data_Task();		
		Server_Xmt2Data_Task();
		
		OS_Kill();
}

	char filename2[8];
//********  Server_RcvData_Task *************** 
// does a task regarding data received from users
// check RECORD, and prepare
void Server_Rcv2Data_Task(void)
{
	int temp=0;
	
// 0) re-organize file_name received	

	for(int i=0; i<7; i++) filename2[i]=NULL;
	for(int i=0; i<7; i++){
		if(Rcv2Data[i+1] == NULL) break;
		filename2[i] =  Rcv2Data[i+1];	
	} 
	//filename2[7] = NULL;
	
	
	for(int i=0; i<8; i++) Xmt2Data[i] = NULL;
	
//1) find record_index and directory_index
	Check_Record(filename2, 2);
	Check_Directory(filename2, 2); // update user2_d_index, whichi is global variable
	
//2) do jobs depending on the command
	switch (Rcv2Data[0]){
		case 0: // ls 								
				
				ls_Task(2);
				temp = ls_total_file_num2 * 8;
				Xmt2Data[0] = '*';
				Xmt2Data[7] = temp & 0x000000FF;
				Xmt2Data[6] = (temp & 0x0000FF00)>>8;
				Xmt2Data[5] = (temp & 0x00FF0000)>>16;
				Xmt2Data[4] = (temp & 0xFF000000)>>24;		
		
				break;

		case 1: // cat/more 
				//a) found in the directory -> send file size 
				if(user2_d_index != 0){
					Xmt2Data[0] = '*';
					Xmt2Data[7] = directory[user2_d_index].size & 0x000000FF;
					Xmt2Data[6] = (directory[user2_d_index].size & 0x0000FF00)>>8;
					Xmt2Data[5] = (directory[user2_d_index].size & 0x00FF0000)>>16;
					Xmt2Data[4] = (directory[user2_d_index].size & 0xFF000000)>>24;
				}
				//b) not found in the directory -> "no file"
				else{
					strcpy((char *)Xmt2Data, "no file");
				}
				break;
		
		case 2: // vi
				//a) found in record
				if(user2_r_index ==100){ //  someone else has it
					strcpy((char *)Xmt2Data, "denied");
				}
				//b-1)  not found in record  && found in directory -> update record
				else{
				//b-2)  not found in record  && not found in directory -> make new file with size 0 (update directory) , update record
					if(user2_d_index == 0){
						  if(eFile_Create(filename2)){
								UART_OutString("eFile_Create Failed"); UART_OutCRLF();
							}
							Check_Directory(filename2,2);	// user2_d_index  is not zero anymore
					}
					// send the size of the file
					Xmt2Data[0] = '*';
					Xmt2Data[7] = directory[user2_d_index].size & 0x000000FF;
					Xmt2Data[6] = (directory[user2_d_index].size & 0x0000FF00)>>8;
					Xmt2Data[5] = (directory[user2_d_index].size & 0x00FF0000)>>16;
					Xmt2Data[4] = (directory[user2_d_index].size & 0xFF000000)>>24;		
					
					// update record
					for(int i=1; i<RECORD_NUM; i++){
						if(record[i].file_name[0] == NULL){
							user2_r_index = i;
							break;
						}
					}
					for(int i=0; i<8; i++) record[user2_r_index].file_name[i] = filename2[i];
					record[user2_r_index].user = 2;
				}
				break;
		
		case 3: // rm   
				//a) not found in directory -> "no file"
				if(user2_d_index ==0){
					strcpy((char *)Xmt2Data, "no file");
				}
				//b-2)  found in directory  && found in record -> "denied"
				else{
					if(user2_r_index !=0){
						strcpy((char *)Xmt2Data, "denied");					
					}
				//b-2)  found in directory  && not found in record -> erase the file, "removed"
					else{ 
						strcpy((char *)Xmt2Data, "removed");		
						if(eFile_Delete(filename2)) {
							UART_OutString("eFile_Delete Failed"); UART_OutCRLF();
						}
					}
				}
				break;
			
		case 4: // disk_format
				strcpy((char *)Xmt2Data, "denied");
				break;
		
		case 5: // wq
				//a) not found in directory -> "no file"
//				if(user2_d_index ==0){
//					strcpy((char *)Xmt2Data, "no file");
//				}
//				//b-1)  found in directory  && found in record -> "saved", and start receiving, update the record
//				else{
					if(user2_r_index ==100){ // someone else has it
						strcpy((char *)Xmt2Data, "denied");
					}
					else if(user2_r_index !=0 ){ // I have it 
						strcpy((char *)Xmt2Data, "saved");
						//update record
						for(int i=0; i<8; i++) record[user2_r_index].file_name[i] =NULL;
						record[user2_r_index].user = 0;						
					}	
				//b-2)  found in directory  && not found in record -> erase the file, "wrong"
					else{
						strcpy((char *)Xmt2Data, "wrong");		
					}
//				}
				break;
			 
		default: 
				strcpy((char *)Xmt2Data, "weird");
	}
}


//******** Server_Xmt2Data_Task  *************** 
//prepare for sending data: cat/more, vi (0, 1, 2) 
//prepare for receiving data: wq (5)								
//or get another command

int Xmt2_num=0;
int Xmt2_done=0;

void Server_Xmt2Data_Task(void){

	Xmt2_num = 0;
	
	CAN0_SendData2(Xmt2Data);

		UART_OutString(" send reponse to USER2: "); UART_OutCRLF();
		UART_OutString("   for ls, cat/more, vi : "); printd(Xmt2Data);
		UART_OutString("   for rm, disk_format, wq: "); printc(Xmt2Data);		
				
	switch (Rcv2Data[0]){
		case 0: // ls   						// prepare to send data
				Xmt2_num = ls_total_file_num2;
				Xmt2_ready=1; 
				break;
		
		case 1: // cat/more 			
				//a) found in the directory -> send file size 
				if(strcmp((char *)Xmt2Data, "no file") == 0){ 
					 if(directory[user2_d_index].size == 0) Xmt2_num =0;
					 NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
					 Xmt2_ready=0; 
				}
				//b) if NOT denied
				else{ // prepare to send data
					Xmt2_num = (directory[user2_d_index].size-1)/8 + 1;
					Xmt2_ready=1; 
				}	
				break;
		
		case 2: // vi
				//a) if denied
				if(strcmp((char *)Xmt2Data, "denied") == 0){ 
					NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
				}
				//b) if NOT denied
				else{ // prepare to send data
					if(directory[user2_d_index].size == 0) Xmt2_num =0;
					else Xmt2_num = (directory[user2_d_index].size-1)/8 +1;
					Xmt2_ready=1; 
				}
				break;
	
		case 3: // rm
				NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
				break;
		
		case 4: // disk_format
				NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
				break;
			 
		case 5: // wq
				if(strcmp((char *)Xmt2Data, "saved") == 0){ // prepare to receive data
					Rcv2_WQ_Task(); 
				}
				else{
					NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
				}
				break;
			 
		default: 
			NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////******* Task only for wq*******//////////////////////////////////////////////////////////


uint32_t wq_can_frame2;
uint32_t wq_file_size2;
//********  Rcv2_WQ_Task (only used for :wq)***************
// receiving the SIZE of the file: wq_new_size
void Rcv2_WQ_Task(void){

  uint8_t data[8];	

	
	CAN0_GetMail2(data);
	
	wq_file_size2 = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7] ; 
	wq_can_frame2 = (wq_file_size2-1)/8 + 1;
	
	UART_OutString("data for User2 wq size : ");	UART_OutUDec(wq_file_size2); UART_OutCRLF();

	// Delete the original file
		if(eFile_Delete(filename2)) {
			UART_OutString("eFile_Delete Failed"); UART_OutCRLF();
		}
	// re-Create the file with same name, but doing this will make the file size and index 0
		if(eFile_Create(filename2)) {
			UART_OutString("eFile_Create Failed"); UART_OutCRLF();
		}	
		
		if(wq_file_size2 == 0)  NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
		else NumCreated += OS_AddThread(Thread_Rcv2_WQ, 128, 2);

}


//********  Thread_Rcv2_WQ (only used for :wq)***************
void Thread_Rcv2_WQ(void){
	
	uint8_t new_file[MAX_FILESIZE];	// file data
	for(int i=0; i<MAX_FILESIZE; i++) new_file[i]=NULL;
	
	uint8_t data[8];	
	
	
	for(int i=0; i<wq_can_frame2; i++){
		
		for(int j=0; j<8; j++) 	data[j] = NULL;
		
		CAN0_GetMail2(data);
		
		UART_OutString("  RECEIVED from User2  ");  UART_OutUDec(i); UART_OutCRLF();
		printc(data);
		
		for(int j=0; j<8; j++){
			new_file[i*8 + j] = data[j];
		}
	}
	
	
	if(eFile_WOpen(filename2)){
			UART_OutString("eFile_WOpen Failed"); UART_OutCRLF();
	}
  for(int i=0;i<wq_file_size2;i++){    
    if(eFile_Write((char)new_file[i])){
			UART_OutString("eFile_Write Failed"); UART_OutCRLF();
		}
  }
	if(eFile_WClose()){   
			UART_OutString("eFile_WClose Failed"); UART_OutCRLF();
	}
		
	UART_OutString(" User2 new file content:  "); 
	for(int i=0; i<wq_file_size2; i++) UART_OutChar(new_file[i]); 
	UART_OutCRLF();
	
	NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
	OS_Kill();
	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////******* Send Task  for ls, more/cat, vi ******////////////////////////////////////////////


//******** PThread_XtmCAN  *************** 
void PThread_XtmCAN(void){
	uint8_t s1data[8] = {0,};
	uint8_t s2data[8]= {0,};
	char data1;// = NULL;
	char data2;// = NULL;
	
		if(Xmt1_ready && (Xmt1_done < Xmt1_num)){
		switch (Rcv1Data[0]){
		
			case 0: // ls   -> send file names
					for(int j=0; j<8; j++){ 
						if(Xmt1_done >0) ls_filenames1[Xmt1_done-1][j] = NULL; //  just to make sure
						s1data[j] = ls_filenames1[Xmt1_done][j];
					}
					break;

			case 1: // cat/more 
					if( Xmt1_done == 0){
						if(eFile_ROpen(filename1)){
							UART_OutString("eFile_ROpen Failed"); UART_OutCRLF();
						}
					}
					for(int j =0; j<8; j++){
						if(eFile_ReadNext(&data1)){
								UART_OutString("eFile_ReadNext Failed"); UART_OutCRLF();
						}
						s1data[j] = data1;
					}
					if( Xmt1_done == Xmt1_num-1){
						if(eFile_RClose()){
							UART_OutString("eFile_RClose Failed"); UART_OutCRLF();
						}
					}
					break;
				
				case 2: // vi    
					if( Xmt1_done == 0){
						if(eFile_ROpen(filename1)){
							UART_OutString("eFile_ROpen Failed"); UART_OutCRLF();
						}
					}
					for(int j =0; j<8; j++){
						if(eFile_ReadNext(&data1)){
								UART_OutString("eFile_ReadNext Failed"); UART_OutCRLF();
						}
						s1data[j] = data1;
					}
					if( Xmt1_done == Xmt1_num-1){
						if(eFile_RClose()){
							UART_OutString("eFile_RClose Failed"); UART_OutCRLF();
						}
					}
					break;
			
				default: 
					NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
			}
			
			CAN0_SendData1(s1data);
			Xmt1_done++;
		
		UART_OutString(" send data to USER1 ("); UART_OutUDec(Xmt1_done); UART_OutString(" )   ");
		printc(s1data);
	
	}
	if((Xmt1_done == Xmt1_num) && Xmt1_ready){
		Xmt1_ready = 0;
		Xmt1_done = 0;
		Xmt1_num = 0;
		for(int i=0; i<8; i++) s1data[i] = 0;
		data1 = NULL;
		
		NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
	}
	
	
	
	
	
	
	if(Xmt2_ready && (Xmt2_done < Xmt2_num)){
		switch (Rcv2Data[0]){
		
			case 0: // ls   -> send file names
					for(int j=0; j<8; j++){ 
						if(Xmt2_done >0) ls_filenames2[Xmt2_done-1][j] = NULL; //  just to make sure
						s2data[j] = ls_filenames2[Xmt2_done][j];
					}
					break;

			case 1: // cat/more 
				if( Xmt2_done == 0){
					if(eFile_ROpen(filename2)){
						UART_OutString("eFile_ROpen Failed"); UART_OutCRLF();
					}
				}
					for(int j =0; j<8; j++){
						if(eFile_ReadNext(&data2)){
								UART_OutString("eFile_ReadNext Failed"); UART_OutCRLF();
						}
						s2data[j] = data2;
					}
				if( Xmt2_done == Xmt2_num-1){
					if(eFile_RClose()){
						UART_OutString("eFile_RClose Failed"); UART_OutCRLF();
					}
				}
					break;
				
				case 2: // vi    
					if( Xmt2_done == 0){
						if(eFile_ROpen(filename2)){
							UART_OutString("eFile_ROpen Failed"); UART_OutCRLF();
						}
					}
					for(int j =0; j<8; j++){
						if(eFile_ReadNext(&data2)){
								UART_OutString("eFile_ReadNext Failed"); UART_OutCRLF();
						}
						s2data[j] = data2;
					}
					if( Xmt2_done == Xmt2_num-1){
						if(eFile_RClose()){
							UART_OutString("eFile_RClose Failed"); UART_OutCRLF();
						}
					}
					break;
			
				default: 
					NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
			}
			
			CAN0_SendData2(s2data);
			Xmt2_done++;
		
		UART_OutString(" send data to USER2: "); UART_OutUDec(Xmt2_done);  UART_OutCRLF();
		printc(s2data);
	
	}
	if((Xmt2_done == Xmt2_num) && Xmt2_ready){
		Xmt2_ready = 0;
		Xmt2_done = 0;
		Xmt2_num = 0;

		for(int i=0; i<8; i++) s2data[i] = 0;
	 data2 = NULL;
		NumCreated += OS_AddThread(Thread_Rcv2CAN, 128, 2);
	}
	
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////******* IDLE*******//////////////////////////////////////////////////////////////////////
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


