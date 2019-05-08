#include <stdint.h>

/**
 * reverseBitOrder
 * ----------
 * Discription: to output in the order of LSB first, we need to reverse all bits.
 */
static uint8_t reverseBitOrder(uint8_t byte);


static void SSI2_SS_HIGH (void);

static void SSI2_SS2_LOW (void);

static void SSI3_SS_HIGH (void);

static void SSI3_SS_LOW (void);

/**
 * Master_SSI_Init
 * SSI2 and SSI3 init
 */
void Master_SSI_Init(void);

/**
 * SSI2_Init
 */
void SSI2_Init(void);

/**
 * SSI3_Init
 */
void SSI3_Init(void);


/**
 * SSI2_read
 * ----------
 * @return date read from slave1 device.
 */
uint16_t SSI2_read (void);

/**
 * SSI2_write
 * ----------
 * @param  data  data to be written.
 */
void SSI2_write(uint16_t data);

/**
 * SSI3_read
 * ----------
 * @return date read from slave2 device.
 */
uint16_t SSI3_read (void);

/**
 * SS3I_write
 * ----------
 * @param  data  data to be written.
 */
void SSI3_write (uint16_t data);



