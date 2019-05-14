/**
 * @file      eFile.h
 * @brief     high-level file system
 * @details   This file system sits on top of eDisk.
 * @version   V1.0
 * @author    Valvano
 * @copyright Copyright 2017 by Jonathan W. Valvano, valvano@mail.utexas.edu,
 * @warning   AS-IS
 * @note      For more information see  http://users.ece.utexas.edu/~valvano/
 * @date      March 9, 2017

 ******************************************************************************/

#include <stdint.h>

typedef struct directory
{
	char file_name[8]; 
	uint32_t  sb;							// start block
	uint32_t  size;						// total number of bytes for this file??? 
}DirType; 									// 16 byte total 

#define SUCCESS 0
#define FAIL 1
                             
//Our SD Card is 32GB = 32 * 2^30 Byte

#define MAX_BLOCKNUM 3000 //4096 // 2^12 byte
#define DIR_BLOCKNUM 2 //4
#define FAT_BLOCKNUM 30 //60

#define BLOCKSIZE 512
#define DIR_BLOCKSIZE 32  // 16byte * 32 = 512, max file num 32*4 = 128
#define FAT_BLOCKSIZE 128 // 4byte * 128 = 512, total FAT num = 30 * 128 = 3840
#define DIR_SIZE DIR_BLOCKNUM* DIR_BLOCKSIZE // 4*32 = 128
// Directory and FAT in RAM 
extern DirType directory[DIR_SIZE];
// Let's say that the directory[0] is for free block	
extern uint32_t fat[FAT_BLOCKNUM*FAT_BLOCKSIZE]; // (x4, because our fat is 4 bytes each)

extern uint8_t rDataBuffer[BLOCKSIZE];
extern uint8_t wDataBuffer[BLOCKSIZE];

extern int wb_index; // index in write buffer
extern int wblock; // the block number to write
extern int rb_index; // index in write buffer
extern int rblock; // the block number to write

extern uint32_t filenum;	// total number of files
extern uint32_t start_free;	// start sector of the free space
extern int rdir_idx;	// opened read file, max size means no read dir
extern int wdir_idx;	// opened write file, max size means no write dir
extern char rfile[8];	// opened read file, 0 if none opened
extern char wfile[8];	// opened write file, 0 if none opened



/**
 * @details This function must be called first, before calling any of the other eFile functions
 * @param  none
 * @return 0 if successful and 1 on failure (already initialized)
 * @brief  Activate the file system, without formating
 */
int eFile_Init(void); // initialize file system


/**
 * @details Erase all files, create blank directory, initialize free space manager
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Format the disk
 */
int eFile_Format(void); // erase disk, add format

/**
 * @details Create a new, empty file with one allocated block
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., already exists)
 * @brief  Create a new file
 */
int eFile_Create( char name[]);  // create new file, make it empty 


/**
 * @details Open the file for writing, read into RAM last block
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Open an existing file for writing
 */
int eFile_WOpen(char name[]);      // open a file for writing 


/**
 * @details Save one byte at end of the open file
 * @param  data byte to be saved on the disk
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Format the disk
 */
int eFile_Write(char data);  

/**
 * @details Deactivate the file system. One can reactive the file system with eFile_Init.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Close the disk
 */
int eFile_Close(void); 


/**
 * @details Close the file, leave disk in a state power can be removed.
 * This function will flush all RAM buffers to the disk.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing to flash)
 * @brief  Close the file that was being written
 */
int eFile_WClose(void); // close the file for writing

/**
 * @details Open the file for reading, read first block into RAM
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Open an existing file for reading
 */
int eFile_ROpen(char name[]);      // open a file for reading 
   

/**
 * @details Read one byte from disk into RAM
 * @param  pt call by reference pointer to place to save data
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Retreive data from open file
 */
int eFile_ReadNext(char *pt);       // get next byte 
                              
/**
 * @details Close the file, leave disk in a state power can be removed.
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., wasn't open)
 * @brief  Close the file that was being read
 */
int eFile_RClose(void); // close the file for writing


/**
 * @details Display the directory with filenames and sizes
 * @param  fp pointer to a function that outputs ASCII characters to display
 * @return 0 if successful and 1 on failure (e.g., trouble reading from flash)
 * @brief  Show directory
 */
int eFile_Directory(void(*fp)(char));

/**
 * @details Delete the file with this name, recover blocks so they can be used by another file
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., file doesn't exist)
 * @brief  delete this file
 */
int eFile_Delete(char name[]);  // remove this file 

/**
 * @details open the file for writing, redirect stream I/O (printf) to this file
 * @note if the file exists it will append to the end<br>
 If the file doesn't exist, it will create a new file with the name
 * @param  name file name is an ASCII string up to seven characters
 * @return 0 if successful and 1 on failure (e.g., can't open)
 * @brief  redirect printf output into this file
 */
int eFile_RedirectToFile(char *name);


/**
 * @details close the file for writing, redirect stream I/O (printf) back to the UART
 * @param  none
 * @return 0 if successful and 1 on failure (e.g., trouble writing)
 * @brief  Stop streaming printf to file
 */
int eFile_EndRedirectToFile(void);
