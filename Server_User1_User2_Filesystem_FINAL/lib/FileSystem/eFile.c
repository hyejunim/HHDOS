// filename ************** eFile.c *****************************
// High-level routines to implement a solid-state disk 
// Jonathan W. Valvano 3/9/17


// to hyejun
// in eFile_WOpen: 1. calculate the last block of the file and store it as "wblock"
//														2. calculate the last block index of the file and store it as "wb_index"
// in eFile_ROpen: 1. calculate the last block of the file and store it as "rblock"
// qestion: can there be two different read files opened at the same time? No (by Justin)
// but why was the last block?


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "eDisk.h"
#include "OS.h"
#include "UART2.h"
#include "eFile.h"

//Sema4Type sUART;


int store_DIR(void);
int store_FAT(void);

// Directory and FAT in RAM 
DirType directory[DIR_SIZE];
// Let's say that the directory[0] is for free block	
uint32_t fat[FAT_BLOCKNUM*FAT_BLOCKSIZE]; // (x4, because our fat is 4 bytes each)

uint8_t rDataBuffer[BLOCKSIZE];
uint8_t wDataBuffer[BLOCKSIZE];

uint8_t bufPt[BLOCKSIZE];

int wb_index=0; // index in write buffer
int wblock = 0; // the block number to write
int rb_index=0; // index in write buffer
int rblock = 0; // the block number to write

uint32_t filenum = 0;	// total number of files
uint32_t start_free = DIR_BLOCKNUM + FAT_BLOCKNUM;	// start sector of the free space
//Sema4Type* sRead1;
//Sema4Type* sWrite1;
Sema4Type* sBufPt;
int rdir_idx = DIR_SIZE;	// opened read file, max size means no read dir
int wdir_idx = DIR_SIZE;	// opened write file, max size means no write dir
char rfile[8] = "";	// opened read file, 0 if none opened
char wfile[8] = "";	// opened write file, 0 if none opened



int get_diskDIR(void)
{
	DWORD i, j, k;
	OS_Wait(sBufPt);
	for(i=0; i<DIR_BLOCKNUM; i++)	// 4 blocks
	{
//		BYTE dir_block[BLOCKSIZE];
		if(eDisk_ReadBlock(bufPt, i)){
			OS_Signal(sBufPt);
			return FAIL;	// read one block of directory
		}
			for(j=0; j<DIR_BLOCKSIZE; j++) // 32 dirs per block
		{
			// get directory name
			for(k=0; k<8; k++) directory[i*DIR_BLOCKSIZE + j].file_name[k] = bufPt[16*j + k];
			
			// get directory sb
			directory[i*DIR_BLOCKSIZE + j].sb =	(bufPt[16*j + 8] << 24) | 
																																(bufPt[16*j + 9] << 16) |
																																(bufPt[16*j + 10] << 8) | 
																																(bufPt[16*j + 11]);  // need to check if it works
			
			// get directory size
			directory[i*DIR_BLOCKSIZE + j].size =	(bufPt[16*j + 12] << 24) | 
																																		(bufPt[16*j + 13] << 16) |
																																		(bufPt[16*j + 14] << 8) | 
																																		(bufPt[16*j + 15]);
		}
	}
	OS_Signal(sBufPt);
	return SUCCESS;
}


int get_diskFAT(void)
{
	DWORD i, j;
	OS_Wait(sBufPt);
	for(i=0; i<FAT_BLOCKNUM; i++)	// 4 blocks
	{
//		BYTE fat_block[BLOCKSIZE];
		if(eDisk_ReadBlock(bufPt, DIR_BLOCKNUM + i)){
				OS_Signal(sBufPt);
				return FAIL;	// read one block of fat
		}
		for(j=0; j<FAT_BLOCKSIZE; j++) // 128 numbers per block
		{
			// get directory sb
			fat[i*FAT_BLOCKSIZE + j] =	(bufPt[4*j + 0] << 24) | 
																									(bufPt[4*j + 1] << 16) | 
																									(bufPt[4*j + 2] << 8) | 
																									(bufPt[4*j + 3]);  // need to check if it works
		}
	}
	OS_Signal(sBufPt);
	return SUCCESS;
}

//---------- eFile_Init-----------------HJ
// Activate the file system, without formating
// Input: none
// Output: 0 if successful and 1 on failure (already initialized)
int eFile_Init(void){ // initialize file system

	if(eDisk_Init(0) == FAIL) return FAIL;
	
	// get DIR and FAT from disk
	if(get_diskDIR() == FAIL) return FAIL;
	if(get_diskFAT() == FAIL) return FAIL;
	
//	OS_InitSemaphore(sRead1, 1);
//	OS_InitSemaphore(sWrite1, 1);
	OS_InitSemaphore(sBufPt, 1);
	
  return SUCCESS;
}

//---------- eFile_Close-----------------HJ
// Deactivate the file system
// Input: none
// Output: 0 if successful and 1 on failure (not currently open)
int eFile_Close(void){ 
	// save all changes to the disk
	if(strcmp(rfile, "") != 0) // read file opened
		if(eFile_RClose() == FAIL) return FAIL;
	
	if(strcmp(wfile, "") != 0) // write file opened
		if(eFile_WClose() == FAIL) return FAIL;
	
	if(store_DIR() == FAIL) return FAIL;
	if(store_FAT() == FAIL) return FAIL;
	
  return SUCCESS;     
}

//---------- eFile_Format-----------------HJ
// Erase all files, create blank directory, initialize free space manager
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Format(void){ // erase disk, add format
	// delete all files
//	for(int i=1; i<DIR_SIZE; i++) // DIR_SIZE=128. directory[0]: start of free space
//	{
//		if(directory[i].file_name[0] != NULL)
//			eFile_Delete(directory[i].file_name);
//	}
	
	// init
	filenum = 0;	// total number of files
	strcpy(rfile, "");	// opened read file, 0 if none opened
	strcpy(wfile, "");	// opened write file, 0 if none opened
	// initialize directory of free space
	directory[0].file_name[0] = NULL;
	directory[0].sb = DIR_BLOCKNUM + FAT_BLOCKNUM;
	directory[0].size = 0;
	for(int i = 1; i<DIR_BLOCKNUM* DIR_BLOCKSIZE; i++) // init dir in RAM
	{
		for(int j=0; j<8; j++)directory[i].file_name[j] = NULL;
		directory[i].sb = 0;
		directory[i].size = 0;
	}
	for(int i = 0; i<FAT_BLOCKNUM*FAT_BLOCKSIZE -1; i++)  
	{
		if(i<DIR_BLOCKNUM+FAT_BLOCKNUM ) fat[i] = 0;
		else fat[i] = i+1;	// init FAT in RAM (make all free space)
	}
	fat[FAT_BLOCKNUM*FAT_BLOCKSIZE -1] = 0;
	if(store_DIR() == FAIL) return FAIL;	// store new dir and fat into the disk
	if(store_FAT() == FAIL) return FAIL;
	
  return SUCCESS;   // OK
}


//---------- eFile_WOpen-----------------HJ
// Open the file, read last block into RAM
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WOpen(char name[]){      // open a file for writing 
	int i;
	
	if(strcmp(rfile, name) == 0) return FAIL;	// another thread is reading the same file
	
//	OS_Wait(sWrite1);
	
	// search the start and size of the file
	for(i = 0; i<DIR_BLOCKNUM* DIR_BLOCKSIZE; i++)
	{
		if(strcmp(directory[i].file_name, name) == 0)	// found the file!
		{
			wdir_idx = i;
			strcpy(wfile, directory[i].file_name); // update wfile
			
			wblock = directory[i].sb;
			while(fat[wblock] != 0)	wblock = fat[wblock];
			wb_index = directory[i].size % BLOCKSIZE; // index of the next empty byte
			if(directory[i].size != 0 && wb_index == 0) wb_index =512;
			
			eDisk_ReadBlock(wDataBuffer, wblock); // write on the first block
			
			break;
		}
	}
	
	if(i == DIR_SIZE)	return FAIL; // no file with such name
	
  return SUCCESS;   
}

//---------- eFile_Write-----------------HS
// save at end of the open file
// Input: data to be saved
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Write(char data){
	int i;
	uint32_t free_start = directory[0].sb;

//if this block is already filled -> find next block to write, and store the current buffer
	if(wb_index == 512){
		//store put this filled up buffer to the DISK
		if(eDisk_WriteBlock(wDataBuffer, wblock) ==FAIL) return FAIL;
		
//find free block and connect to fat
		fat[wblock] = free_start;
		directory[0].sb = fat[free_start];
		fat[free_start]=0 ;

	//update
		wb_index = 0;
		wblock = free_start;
		for(i=0; i<BLOCKSIZE; i++){
			wDataBuffer[i] = 0;
		}
	}
	//if(wb_index > 512) printf("something wrong"); 
	
// start writing one byte to the buffer
	wDataBuffer[wb_index] = data;
	directory[wdir_idx].size = directory[wdir_idx].size + 1; // size of this file is increased by 1 
	wb_index++;
	
  return SUCCESS;    
}


//---------- eFile_WClose-----------------HJ
// close the file, left disk in a state power can be removed
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_WClose(void){ // close the file for writing
  
	// update file size <- did this in eFile_Write
	// directory[wdir_idx].size += wb_index;
	
	// save all written things on RAM to disk
	if(store_DIR() == FAIL) return FAIL;
	if(store_FAT() == FAIL) return FAIL;
	if(eDisk_WriteBlock(wDataBuffer, wblock) == FAIL)
		return FAIL;
	
	// release wfile
	wdir_idx = DIR_SIZE;
	wb_index = 0;
	strcpy(wfile, "");
//	OS_Signal(sWrite1);
	
  return SUCCESS;
}


//---------- eFile_ROpen-----------------HJ
// Open the file, read first block into RAM 
// Input: file name is a single ASCII letter
// Output: 0 if successful and 1 on failure (e.g., trouble read to flash)
int eFile_ROpen( char name[]){      // open a file for reading 
	int i;
	DWORD a;
	
	if(strcmp(wfile, name) == 0) return FAIL;	// another thread is writing the same file
	
//	OS_Wait(sRead1);
	
	
	// search the start and size of the file
	for(i = 0; i<DIR_SIZE; i++)
	{
		if(strcmp(directory[i].file_name, name) == 0)	// found the file!
		{
			rdir_idx = i;
			strcpy(rfile, directory[i].file_name); // update rfile
			a = directory[i].sb;
			if(eDisk_ReadBlock(rDataBuffer, a)==FAIL) return FAIL; // read on the first block
			
			rblock = directory[i].sb;
			rb_index = 0;
			break;
		}
	}
	
	if(i == DIR_SIZE)	return FAIL; // no file with such name
  return SUCCESS;
}
 

//---------- eFile_ReadNext-----------------HS
// retreive data from open file
// Input: none
// Output: return by reference data
//         0 if successful and 1 on failure (e.g., end of file)
int eFile_ReadNext( char *pt){       // get next byte 

//if this block is all read-> find next block to read
  if(rb_index >= 512){
		// if this was the last block of the file
    if(fat[rblock] == 0){
			return FAIL;
		}

		//find next block to read
		rblock = fat[rblock];
			
		//store this new block to buffer
		if(eDisk_ReadBlock(rDataBuffer,rblock) == FAIL) return FAIL;
		
		//update
		rb_index = 0;
	}
	
	 // if this is the last block, last index of the file
	
  *pt = rDataBuffer[rb_index];
  rb_index++;
/*	 
	if((fat[rblock]==0) && (directory[rdir_idx].size%512 ==rb_index-1)){
    return FAIL;
  }
	*/
  return SUCCESS; 
}

    
//---------- eFile_RClose-----------------HJ
// close the reading file
// Input: none
// Output: 0 if successful and 1 on failure (e.g., wasn't open)
int eFile_RClose(void){ // close the file for writing

	// release rfile
	rdir_idx = DIR_SIZE;
	rb_index = 0;
	strcpy(rfile, "");
//	OS_Signal(sRead1);
	
  return SUCCESS;
}




//---------- store_DIR-----------------HS
// Store the DirType directory to the DISK 
// Input: none
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int store_DIR(void)
{
//  uint8_t bufPt[512]; // byte size buffer
  uint8_t result;
	OS_Wait(sBufPt);
	
  for(int i=0; i<DIR_BLOCKNUM; i++){ 
		//0) dataBuffer is the address of the data to be written  
    for(int j=0; j<DIR_BLOCKSIZE; j++){
			//1) put file_name(8byte) in the buffer
      for(int k=0; k<8; k++){																												
        bufPt[16*j+k] = directory[i*DIR_BLOCKSIZE+j].file_name[k];
      }
			//2) put sb(4byte) in the buffer
      bufPt[16*j+8] = (directory[i*DIR_BLOCKSIZE+j].sb & 0xFF000000)>>24; 
			bufPt[16*j+9] = (directory[i*DIR_BLOCKSIZE+j].sb & 0x00FF0000)>>16; 
			bufPt[16*j+10] = (directory[i*DIR_BLOCKSIZE+j].sb & 0x0000FF00)>>8; 
      bufPt[16*j+11] =  directory[i*DIR_BLOCKSIZE+j].sb & 0x000000FF;
			//3) put size(4byte) in the buffer
      bufPt[16*j+12] = (directory[i*DIR_BLOCKSIZE+j].size & 0xFF000000)>>24;
			bufPt[16*j+13] = (directory[i*DIR_BLOCKSIZE+j].size & 0x00FF0000)>>16;
			bufPt[16*j+14] = (directory[i*DIR_BLOCKSIZE+j].size & 0x0000FF00)>>8;
      bufPt[16*j+15] = directory[i*DIR_BLOCKSIZE+j].size & 0x000000FF;
			//4) 
    }
    result = eDisk_WriteBlock(bufPt, i);
    if(result != RES_OK){
				OS_Signal(sBufPt);
				return FAIL;
		}
  }
	OS_Signal(sBufPt);
  return SUCCESS;
}


int store_FAT(void)
{
	uint8_t result;
//	uint8_t bufPt[512]; 
	OS_Wait(sBufPt);
	for(int i=0; i<FAT_BLOCKNUM; i++){
		for(int j=0; j<FAT_BLOCKSIZE;j++){
			bufPt[4*j] = (fat[i*FAT_BLOCKSIZE+j] & 0xFF000000)>>24; 
			bufPt[4*j+1] = (fat[i*FAT_BLOCKSIZE+j] & 0x00FF0000)>>16; 
			bufPt[4*j+2] = (fat[i*FAT_BLOCKSIZE+j] & 0x0000FF00)>>8; 
			bufPt[4*j+3] = (fat[i*FAT_BLOCKSIZE+j] & 0x000000FF); 
		}
		result = eDisk_WriteBlock(bufPt, i+DIR_BLOCKNUM);
		if(result != RES_OK){
			OS_Signal(sBufPt);
			return FAIL;
		}
	}
	OS_Signal(sBufPt);
	
	return SUCCESS;
}


//---------- eFile_Create-----------------HS
// Create a new, empty file with one allocated block
// Input: file name is an ASCII string up to seven characters 
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Create( char name[8]){  // create new file, make it empty 
	
	int i;
	int dir_index;
	
	uint32_t free_start = directory[0].sb;

	// 1) find directory space that is not being used
	for(i=1; i< DIR_BLOCKNUM * DIR_BLOCKSIZE; i++){
		if(directory[i].file_name[0] == NULL){
			dir_index = i;
			break;
		}
	}	
	// 2) update directory
	for(int j=0; j<8; j++){
		directory[dir_index].file_name[j] = name[j];
	}
  directory[dir_index].sb = free_start;
  directory[dir_index].size = 0;
	
	// 3) update directory[0].sb and fat (because it's not a free block anymore)	
	directory[0].sb = fat[free_start];
	fat[free_start] = 0;
	
	// 4) update real directory in DISK
	if(store_DIR() ==FAIL) return FAIL;
	
	// 5) update real fat in DISK
	if(store_FAT() ==FAIL) return FAIL;
	
  return SUCCESS;     
}


//---------- eFile_Delete-----------------HS
// delete this file
// Input: file name is a single ASCII letter---------------------------------->????
// Output: 0 if successful and 1 on failure (e.g., trouble writing to flash)
int eFile_Delete( char name[]){  // remove this file

	int i;
	
	int free_dir_index;
	uint32_t free_start = directory[0].sb;
	uint32_t last_block;

	
	//1) find the file to delete
	for(i=1; i<DIR_BLOCKNUM*DIR_BLOCKSIZE; i++){
		if( strcmp(name, directory[i].file_name) == 0){
			free_dir_index = i;
			break;
		}
	}
	if(i == DIR_BLOCKNUM*DIR_BLOCKSIZE) return FAIL;
	
	
	
	
	
 // 2) update directory[0].sb and fat -> add all blocks to free block linked list, add in front of free_start
	directory[0].sb = directory[free_dir_index].sb;
	
	last_block = directory[free_dir_index].sb;
	while(fat[last_block] != 0){
		last_block = fat[last_block];		
	}
	fat[last_block] = free_start;
	// 3) update directory-> reset stuff in directory
//	strcpy(directory[free_dir_index].file_name, "");
	directory[free_dir_index].file_name[0] = NULL;
	directory[free_dir_index].file_name[1] = NULL;
	directory[free_dir_index].file_name[2] = NULL;
	directory[free_dir_index].file_name[3] = NULL;
	directory[free_dir_index].file_name[4] = NULL;
	directory[free_dir_index].file_name[5] = NULL;
	directory[free_dir_index].file_name[6] = NULL;
	directory[free_dir_index].file_name[7] = NULL;
  directory[free_dir_index].sb = 0;
  directory[free_dir_index].size = 0;	
	
	// 4) update real directory in DISK
	if(store_DIR() == FAIL) return FAIL;
	// 5) update real fat in DISK
	if(store_FAT() ==FAIL) return FAIL;
	
		return SUCCESS;    // restore directory back to flash
}


//---------- eFile_Directory-----------------HS
// Display the directory with filenames and sizes
// Input: pointer to a function that outputs ASCII characters to display
// Output: none
//         0 if successful and 1 on failure (e.g., trouble reading from flash)
int eFile_Directory(void(*fp)(char)){   

	int i,j;
	char array[10];
	uint32_t temp;
	int index;
  
	OS_Wait(&sUART);
	for(i=1; i<DIR_BLOCKNUM*DIR_BLOCKSIZE; i++){
		if(directory[i].file_name[0] != NULL){ // if there is a file in this directory slot
			//print out file name
			//printf("file_name: ");
			for(j=0; j<8; j++){
				fp(directory[i].file_name[j]);
				if(directory[i].file_name[j]==0) break;
			}
			
			fp(32);	// space
			//print out file size: depends on how many blocks this file is occupying
			index =0;
			temp = directory[i].size;
			while(temp != 0){
				array[index] = temp%10;
				temp= temp/10;
				index++;
			}
			for(j=index-1; j>=0; j--){
				fp(array[j]+48);
			}
			
			fp(9); // tab
		}
	}
	
	OS_Signal(&sUART);
	
  return SUCCESS;
}




int StreamToFile=0;                // 0=UART, 1=stream to file

int eFile_RedirectToFile(char *name){
  eFile_Create(name);              // ignore error if file already exists
  if(eFile_WOpen(name)) return 1;  // cannot open file
  StreamToFile = 1;
  return 0;
}

int eFile_EndRedirectToFile(void){
  StreamToFile = 0;
  if(eFile_WClose()) return 1;    // cannot close file
  return 0;
}

int fputc (int ch, FILE *f) { 
  if(StreamToFile){
    if(eFile_Write(ch)){          // close file on error
       eFile_EndRedirectToFile(); // cannot write to file
       return 1;                  // failure
    }
    return 0; // success writing
  }

   // regular UART output
  UART_OutChar(ch);
  return 0; 
}

int fgetc (FILE *f){
  char ch = UART_InChar();  // receive from keyboard
  UART_OutChar(ch);            // echo
  return ch;
}
