#include <stdint.h>

/**
 * reverseBitOrder
 * ----------
 * Discription: to output in the order of LSB first, we need to reverse all bits.
 */
static uint8_t reverseBitOrder(uint8_t byte);


static void SSI3_SS_HIGH (void);

static void SSI3_SS_LOW (void);

static void SS1_SS_HIGH (void);

static void SS1_SS_LOW (void);



/**
 * Slave2_SSI_Init
 * SSI3 and SSI1 init
 */
void Slave2_SSI_Init(void);

/**
 * SSI3_Init
 * Communication with the master
 * Slave configuration
 */
void SSI3_Init(void);

/**
 * SSI1_Init
 * Communication with the slave1
 * Slave configuration
 */
void SSI1_Init(void);


/**
 * SSI3_read
 * ----------
 * @return date read from slave device.
 */
uint16_t SSI3_read (void);

/**
 * SS3I_write
 * ----------
 * @param  data  data to be written.
 */
void SSI3_write (uint16_t data);

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

