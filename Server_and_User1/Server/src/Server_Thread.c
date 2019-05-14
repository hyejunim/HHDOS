#include <stdint.h>
#include <string.h>
#include "ST7735.h"
#include "can0.h"
#include "OS.h"
#include "Server_Thread.h"

#include "Interpreter.h"
#include "UART2.h"

#define RECORD_NUM 10

//////////////////////////////////////////////////////////////
//typedef struct directory
//{
//	char file_name[8]; 
////	uint32_t  sb;							// start block
//	uint32_t  size;						// total number of bytes for this file
//}FILE;

//FILE files[4];


//////////////////////////////////////////////


uint8_t a[32] = "hello!!";
uint8_t b[32] = "how are you?!?!";
uint8_t c[32] = "I am fine, how about u?";
uint8_t d[32] = "This RTOS project is too hard!!";

uint32_t filesize[5];
char filename[5][8];
	

uint32_t all_filename_size = 4*8;
uint32_t all_file_size = 80;
uint32_t all_file_num = 4;

/////////////////////////////////////////////////////*******RECORD*******//////////////////////////////////////////////////////////////
//******** struct for record  *************** 
typedef struct Table
{
	char file_name[8];
	uint8_t user; // who is in charge of this file: 0-server, 1-user1, 2-user2
	
} RECORD;

RECORD record[RECORD_NUM];

	
//******** Record_Init  *************** 
void Record_Init(void){
	
	// only for this example, also reset other things:
	filesize[1] = 8;
	filesize[2] = 16;
	filesize[3] = 24;
	filesize[4] = 32;
	
	filename[1][0]= 'a';
	filename[2][0] = 'b';	
	filename[3][0] = 'c';
	filename[4][0] = 'd';
	
	
	
	
	for(int i=0; i<RECORD_NUM; i++){
		record[i].file_name[0] = 0;
	}
}


//////////******** Record_VI *************** 
/////////// here, we have to make sure that "record" is never full!!!!!!!!
/////////// also, record[0] is not used!!!!!!!!!!!!!!!!!!
/////////// also, directory[0] is not used!!!!!!!!!!!!!!!!!!-> free_space
//////// int USER1_directory_index = 0;


////////int Record_VI(char * name, uint8_t user){

////////	int record_index;
////////	int i,j;
////////	
////////	for(i=1; i<MAX_FILENUM; i++){
////////		if(strcmp(record[i].file_name, name) == 0)	// found the file in the record
////////		{
////////			break;
////////		}
////////	}
////////	
////////	
////////	// file not found in the record
////////	if(i==MAX_FILENUM){ 			
////////		// 1) check if the file exist
////////		for(j=1; j<= all_file_num; j++){
////////			if(strcmp(filename[j], name) == 0){ // found the file in the library
////////				USER1_directory_index = j;
////////				break;
////////			}
////////		}
////////		// 1-1) it's a new file -> update directory and record
////////		if( j > all_file_num){
////////				// find the empty directory
////////				// update the directory.filename, directory.sb and directory.size as 0
////////			  //USER1_directory_index = something
////////				
////////		}

////////		// 1-2) file exist, but not taken by anyone -> just update record
////////		
////////		// 2) what index should this record go?
////////		for(int j=1; j<MAX_FILENUM; j++){
////////			if(record[i].file_name[0] == 0){
////////				record_index = j;
////////				break;
////////			}
////////		}
////////		//write in the record, and let it take the file
////////		strcpy(record[record_index].file_name, name);
////////		record[record_index].user = user;
////////		
////////		return USER1_directory_index;
////////	}
////////	else{ // file found in the record
////////		return 0;
////////	}
////////	

////////}


//////////******** Record_RM *************** 
////////////////////////////////////////////////////////////// temporary
////////int Record_RM(char * name){

////////	// find if this file is in the record
////////		return 1; // fail to rm
////////	
////////	// if file is not in the record
////////			// 1) file does not exist
////////					//return 1;
////////	
////////			// 2) file does exist, but not taken
////////					// delete from the directory
////////					// return 0;


////////}

//////////******** Record_WQ *************** 
////////int Record_WQ(char * name){

////////	int record_index;
////////	int i;
////////	
////////	for(i=1; i<MAX_FILENUM; i++){
////////		if(strcmp(record[i].file_name, name) == 0)	// found the file in the record
////////		{
////////			record_index = i;
////////			break;
////////		}
////////	}
////////	
////////	
////////	// file not found in the record -> cannot do wq
////////	if(i==MAX_FILENUM){ 			
////////		return 0;
////////	}
////////	
////////	// file found in the record -> erase record
////////	else{ 
////////		// update record
////////		record[record_index].file_name[0] =0; 
////////		record[record_index].user = 0;
////////		return 1;
////////	}
////////	
////////}

//******** Check_Record ***************
// check record and return the record_index if the file exist in the record
// 															return 0 if the file does not exist
// record[0] is empty....... ALWAYS
int user1_r_index = 0;
void Check_Record(char * name, uint8_t user){
	int i;

	for(i=1; i< RECORD_NUM; i++){
		if(strcmp(record[i].file_name, name) == 0){ // found the file in the record
			user1_r_index=i;
			break;
		}
	}
	if(i > all_file_num){ // not in record
		user1_r_index = 0;
	}
}

//******** Check_Directory ***************
// check directory and update the user1_d_index if the file exist in the directory
// 																update the user1_d_index to 0  if the file  does not exist
// direcotory[0] is empty.......

int user1_d_index = 0;
void Check_Directory(char * name){
	
	int i;
	for(i=1; i<= all_file_num; i++){ //  all_file_num will be "< DIR_SIZE"
		if(strcmp(filename[i], name) == 0){ // found the file in the directory 
			user1_d_index=i;
			break;
		}
	}
	// not in directory
	if(i > all_file_num){ // if(i == DIR_SIZE){ 
		user1_d_index = 0;
	}
}



////////////////////////////////////////////////////*******General Task for Server*******////////////////////////////////////////////////////////////////

//		|	command	|						14byte CAN        ->      8byte CANsss			| record update
//---------------------------------------------------------------------------
//0 | ls 								| send (* + file_size) 				-> all file_names					| not "check out"
//1 | cat/more 		| send (* + file_size) 				-> file 												 	| not "check out"
//2 | vi									| send (* + file_size) 				-> file  													| "check out"
//		|										|		 or ("access denied) 																		| none
//3 | rm								| send ("access denied")																		| when it's "checked out"
//   | 										|     or send ("remove sucess")														| when it's not "checked out"
//4 | disk_format	| send ("access denied")																		| none
//5 | :wq							| send ("not existing")																					| none
//   |        						|     or send ("file save")																				| "return"


uint8_t Rcv1Data[8];
uint8_t Xmt1Data[8];
uint8_t Xmt1_ready=0;
//******** Thread_Rcv1CAN  *************** 
// receiving data sent by USER1
void Thread_Rcv1CAN(void)
{
  uint8_t data[8];
	Xmt1_ready=0;

//////////////////////////debug///////////////////////	
	UART_OutCRLF(); UART_OutCRLF();
	UART_OutString("////****START new COMMAND****///"); UART_OutCRLF();
   UART_OutString(" waiting to receive....."); UART_OutCRLF();
///////////////////////////////////////////////////////	
	
		CAN0_GetMail1(data);
		for(int i=0; i<8; i++){
				Rcv1Data[i]=data[i];
		}
//////////////////////////debug///////////////////////		
		UART_OutString(" receive command data: ");
		UART_OutUDec(Rcv1Data[0]);
		for(int i=1; i<8; i++){
			UART_OutChar((char)Rcv1Data[i]);
		}
		UART_OutCRLF();
////////////////////////////////////////////////////////		
		
		Server_Rcv1Data_Task();		
		Server_Xmt1Data_Task();
		


		OS_Kill();
}


//********  Server_RcvData_Task *************** 
// does a task regarding data received from users
// check RECORD, and prepare


void Server_Rcv1Data_Task(void)
{
	
	
	int i;
	int new_r_index;
	int new_d_index;
	
// 0) re-organize file_name received	
	char temp_name[8];
	for(int i=0; i<7; i++){
		temp_name[i] =  Rcv1Data[i+1];	
	}
	temp_name[7] = 0;
	
//1) find record_index and directory_index
	Check_Record(temp_name, 1);
	Check_Directory(temp_name); // update user1_d_index, whichi is global variable
	
//2) do jobs depending on the command
	switch (Rcv1Data[0]){
		case 0: // ls 								//////////////////////////////////////////this is temporary 
				Xmt1Data[0] = '*';
				Xmt1Data[7] = all_filename_size & 0x000000FF;
				Xmt1Data[6] = (all_filename_size & 0x0000FF00)>>8;
				Xmt1Data[5] = (all_filename_size & 0x00FF0000)>>16;
				Xmt1Data[4] = (all_filename_size & 0xFF000000)>>24;		
				break;

		case 1: // cat/more 			//////////////////////////////////////////this is temporary
				//a) found in the directory -> send file size 
				if(user1_d_index != 0){
					Xmt1Data[0] = '*';
					Xmt1Data[7] = filesize[user1_d_index] & 0x000000FF;
					Xmt1Data[6] = (filesize[user1_d_index] & 0x0000FF00)>>8;
					Xmt1Data[5] = (filesize[user1_d_index] & 0x00FF0000)>>16;
					Xmt1Data[4] = (filesize[user1_d_index] & 0xFF000000)>>24;
				}
				//b) not found in the directory -> "no file"
				else{
					strcpy((char *)Xmt1Data, "no file");
				}
				break;
		
		case 2: // vi
				//a) found in record
				if(user1_r_index !=0){
					strcpy((char *)Xmt1Data, "denied");
				}
				//b-1)  not found in record  && found in directory -> update record
				else{
					if( user1_d_index != 0){ 
						new_d_index = user1_d_index;
					}	
				//b-2)  not found in record  && not found in directory -> make new file with size 0 (update directory) , update record
					else{
						for(i=1; i<= all_file_num; i++){ //  all_file_num will be "< DIR_SIZE"
							if(filename[i][0] == 0){ // directory[i].file_name[0]
								new_d_index = i;
								break;
							}
						}
						strcpy(filename[new_d_index], temp_name);
						filesize[new_d_index] = 0;
						// have to put sb as well.... LATER
					}
					// update record
					for(int i=1; i<RECORD_NUM; i++){
						if(record[i].file_name[0] == 0){
							new_r_index = i;
							break;
						}
					}
					strcpy(record[new_r_index].file_name, temp_name);
					record[new_r_index].user = 1;
					// send the size of the file
					Xmt1Data[0] = '*';
					Xmt1Data[7] = filesize[new_d_index] & 0x000000FF;
					Xmt1Data[6] = (filesize[new_d_index] & 0x0000FF00)>>8;
					Xmt1Data[5] = (filesize[new_d_index] & 0x00FF0000)>>16;
					Xmt1Data[4] = (filesize[new_d_index] & 0xFF000000)>>24;
									
					user1_d_index = new_d_index;
					user1_r_index = new_r_index;
					
				}
				break;
		
		case 3: // rm   //////////////////////////////////////////this is temporary 
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
						for(i=0; i<8; i++){
							filename[user1_d_index][i] = 0;
						}
						filesize[user1_d_index]=0;
						// free the sb as well.. later
					}
				}
				break;
			
		case 4: // disk_format
				strcpy((char *)Xmt1Data, "denied");
				break;
		
		case 5: // wq
				//a) not found in directory -> "no file"
//////////////////////////////////////////////		
		UART_OutString(" wq file name: "); 
		UART_OutString(filename[user1_d_index]); UART_OutCRLF();		
////////////////////////////////////////////////////		
				if(user1_d_index ==0){
					strcpy((char *)Xmt1Data, "no file");
				}
				//b-1)  found in directory  && found in record -> "saved", and start receiving, update the record
				else{
					if(user1_r_index !=0){
						strcpy((char *)Xmt1Data, "saved");
						for(i=0; i<8; i++){
							record[user1_r_index].file_name[i] =0;
						}
						record[user1_r_index].user = 0;
						
						
						
					}
				//b-2)  found in directory  && not found in record -> erase the file, "wrong"
					else{
						strcpy((char *)Xmt1Data, "wrong");		
					}
				}
				break;
			 
		default: 
				strcpy((char *)Xmt1Data, "default");
	}
}


//******** Server_Xmt1Data_Task  *************** 
//after this Task, if ls, cat/more, vi (0, 1, 2)	 -> keep sending (file(8byte)) 
//after this Task, if wq (5)														-> keep receiving (file_size(8btye) -> file(8byte)) 
//after this Task, if rm, disk_format (3,4)       -> keep receiving (command(8byte))
//																																		->  general thread, and everything at some point get here
int Xmt1_num=0;
int Xmt1_done=0;

void Server_Xmt1Data_Task(void){

	
	
	Xmt1_num = 0;
	
	CAN0_SendData1(Xmt1Data);
/////////////////////////////////////////	
		UART_OutString(" send reponse data: "); UART_OutCRLF();
		UART_OutString("   for ls, cat/more, vi : "); 
		UART_OutChar((char)Xmt1Data[0]);
		for(int i=1; i<8; i++){
			UART_OutUDec((char)Xmt1Data[i]);
		}
		UART_OutCRLF(); 
		UART_OutString("   for rm, disk_format, wq: "); 
		for(int i=0; i<8; i++){
			UART_OutChar((char)Xmt1Data[i]);
		}
		UART_OutCRLF();
//////////////////////////////////////////	
		
				
	switch (Rcv1Data[0]){
		case 0: // ls   						
				Xmt1_num = all_filename_size/8;
				Xmt1_ready=1; 
				Xmt1_done = 0;
				break;
		
		case 1: // cat/more 			
				//a) found in the directory -> send file size 
				if(user1_d_index != 0){
					Xmt1_num = filesize[user1_d_index]/8;
					Xmt1_ready=1; 
				}
				//b) not found in the directory -> "no file"
				else{
					 NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
				}
				break;
		
		case 2: // vi
				//a) if it was denied
				if(strcmp((char *)Xmt1Data, "denied") == 0){ 
					NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
				}
				//b) if it was NOT denied
				else{
					Xmt1_num = filesize[user1_d_index]/8;
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
				if(strcmp((char *)Xmt1Data, "saved") == 0){
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


uint32_t wq_new_size=0;

//********  Rcv1CAN14B_Task (only used for :wq)***************
void Rcv1_WQ_Task(void){
	
	
  uint8_t data[8];	
	// receive size
	CAN0_GetMail1(data);
	
	wq_new_size = (data[4] << 24) + (data[5] << 16) + (data[6] << 8) + data[7] ; 
	
//////////////////////////debug///////////////////////	
	UART_OutString(" received wq size: "); 
	UART_OutUDec(data[4]);UART_OutString(" ");
	UART_OutUDec(data[5]);UART_OutString(" ");
	UART_OutUDec(data[6]);UART_OutString(" ");
	UART_OutUDec(data[7]);UART_OutCRLF();
////////////////////////////////////////////////////////////////	

	
	// find the directory and edit the size
	filesize[user1_d_index] = wq_new_size;

	
	// add Rcv1CAN8B_Thread
	NumCreated += OS_AddThread(Thread_Rcv1_WQ, 128, 2);

}


//********  Thread_Rcv1_WQ (only used for :wq)***************
void Thread_Rcv1_WQ(void){
	uint8_t new_file[32];	
	
	uint8_t data[8];	
	
//////////////////////////debug///////////////////////	
	UART_OutString(" waiting to receive new wq file of SIZE: ");  
	UART_OutUDec(filesize[user1_d_index]);UART_OutCRLF();
///////////////////////////////////////////////////////////////
	
	for(int i=0; i<(wq_new_size/8); i++){
		
//////////////////////////debug///////////////////////		
		UART_OutString("   start: "); UART_OutUDec(i); 
///////////////////////////////////////////////////////////////////////////////////
		
		CAN0_GetMail1(data);
		
		
//////////////////////////debug///////////////////////		
		UART_OutString("done! :  "); 
///////////////////////////////////////////////////////////////////////////////////

		
		for(int j=0; j<8; j++){
			new_file[i*8 + j] = data[j];
//////////////////////////debug///////////////////////
			UART_OutChar((char) data[j]);
/////////////////////////////////////////////////////////			
		}
//////////////////////////debug///////////////////////
			UART_OutCRLF();
/////////////////////////////////////////////////////////		
	}
	
//////////////////////////debug///////////////////////
	UART_OutString(" received new wq file :  "); 
	UART_OutString((char*) new_file); UART_OutCRLF();
////////////////////////////////////////////////////
	
	
	// when this thread is done, it has to be saved in the directory as well, sb........
	if(user1_d_index == 1){
		for(int i=0; i<32; i++)	a[i] = new_file[i];
	}
	if(user1_d_index == 2){
		for(int i=0; i<32; i++)	b[i] = new_file[i];
	}	
	if(user1_d_index == 3){
		for(int i=0; i<32; i++)	c[i] = new_file[i];
	}	
	if(user1_d_index == 4){
		for(int i=0; i<32; i++)	d[i] = new_file[i];
	}
	
	NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
	OS_Kill();
	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////******* Send Task  for ls, more/cat, vi ******////////////////////////////////////////////


//******** PThread_XtmCAN  *************** 
void PThread_XtmCAN(void){
	uint8_t s1data[8];
	if(Xmt1_ready && (Xmt1_done < Xmt1_num)){
		switch (Rcv1Data[0]){
			case 0: // ls   -> send filename
					for(int i=0; i<8; i++){ /////////////////////////////// temporary
						s1data[i] = filename[(Xmt1_done+1)][i];
					}
				break;

		case 1: // cat/more  /////////////////////////////// temporary

				if(user1_d_index==1){
						for(int i=0; i<8; i++){ 
							s1data[i] = a[i + Xmt1_done*8];
					 }
				}
				if(user1_d_index ==2){
						for(int i=0; i<8; i++){ 
							s1data[i] = b[i+ Xmt1_done*8];
					 }
				}
				if(user1_d_index ==3){
						for(int i=0; i<8; i++){ 
							s1data[i] = c[i+ Xmt1_done*8];
					 }					
				}
				if(user1_d_index == 4){
						for(int i=0; i<8; i++){ 
							s1data[i] = d[i+ Xmt1_done*8];
					 }
				}
			break;
			
			case 2: // vi    // filesize[index] 
				if(user1_d_index==1){
						for(int i=0; i<8; i++){ 
							s1data[i] = a[i + Xmt1_done*8];
					 }
				}
				if(user1_d_index ==2){
						for(int i=0; i<8; i++){ 
							s1data[i] = b[i+ Xmt1_done*8];
					 }
				}
				if(user1_d_index ==3){
						for(int i=0; i<8; i++){ 
							s1data[i] = c[i+ Xmt1_done*8];
					 }					
				}
				if(user1_d_index == 4){
						for(int i=0; i<8; i++){ 
							s1data[i] = d[i+ Xmt1_done*8];
					 }
				}
				break;
		
			default: 
				NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
		}
		CAN0_SendData1(s1data);
		
		
//////////////////////////debug///////////////////////
		UART_OutString(" send periodic data: "); UART_OutUDec(Xmt1_done);
		UART_OutString("  :  "); 
		for(int i=0; i<8; i++){
			UART_OutChar((char)s1data[i]);
		}
		UART_OutCRLF();
////////////////////////////////////////////////////////
		
		Xmt1_done++;
	}
	
	if((Xmt1_done == Xmt1_num) && Xmt1_ready){
		Xmt1_ready = 0;
		Xmt1_done = 0;
		Xmt1_num = 0;
		NumCreated += OS_AddThread(Thread_Rcv1CAN, 128, 2);
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


