/******** Slave1_SSI ********
* Author: Hyejun Im (hi956), Hyunsu Chae (hc25673)
* SSI communication for the King
* Slave1 communicates with Master and Slave2
*
* <SSI port usage>
* Slave1 - Master:	SSI2 (PB4-ssi2clk, PB5-ssi2fss, PB6-ssi2RX, PB7-ssi2TX)
* Slave1 - Slave2:	SSI3 (PF0-ssi2clk, PF1-ssi2fss, PF2-ssi2RX, PF3-ssi2TX)
*
* Credit : Zee Lv
*/

#ifndef __SSI1_H__
#define __SSI1_H__

#ifndef __SSI2_H__
#define __SSI2_H__

#include <stdint.h>

/*
 *  SSI2 B Conncection with King
 *  ------------------
 *  SCK  --------- PB4
 *  SS   --------- PB5
 *  MISO --------- PB6
 *  MOSI --------- PB7
 *
 *  SSI1 F Conncection with Slave2
 *  ------------------
 *  SCK  --------- PF0
 *  SS   --------- PF1
 *  MISO --------- PF2
 *  MOSI --------- PF3
 */

/****************************************************
 *                                                  *
 *                 Helper Functions                 *
 *                                                  *
 ****************************************************/

/**
 * reverseBitOrder
 * ----------
 * Discription: to output in the order of LSB first, we need to reverse all bits.
 */
static uint8_t reverseBitOrder(uint8_t byte);

/****************************************************
 *                                                  *
 *                   SS Functions                   *
 *                                                  *
 ****************************************************/
static void SSI2_SS_HIGH (void);

static void SSI2_SS_LOW (void);

static void SS1_SS_HIGH (void);

static void SS1_SS_LOW (void);


/****************************************************
 *                                                  *
 *                   Initializer                    *
 *                                                  *
 ****************************************************/
/**
 * Slave2_SSI_Init
 * SSI3 and SSI1 init
 */
void Slave1_SSI_Init(void);

/**
 * SSI2_Init
 * ----------
 * @brief initialize SSI with corresponding setting parameters.
 */
void SSI2_Init(void);


/**
 * SSI1_Init
 * ----------
 * @brief initialize SSI with corresponding setting parameters.
 */
void SSI1_Init(void);
/****************************************************
 *                                                  *
 *                   I/O Functions                  *
 *                                                  *
 ****************************************************/

/**
 * SSI1_read
 * ----------
 * @return date read from slave device.
 */
uint16_t SSI1_read (void);

/**
 * SSI1_write
 * ----------
 * @param  data  data to be written.
 */
void SSI1_write(uint16_t data);



/**
 * SSI2_read
 * ----------
 * @return date read from slave device.
 */
uint16_t SSI2_read (void);

/**
 * SSI2_write
 * ----------
 * @param  data  data to be written.
 */
void SSI2_write(uint16_t data);

#endif

#endif
