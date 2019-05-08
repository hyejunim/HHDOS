/******** Slave2_SSI ********
* Author: Hyejun Im (hi956), Hyunsu Chae (hc25673)
* SSI communication for the King
* Slave1 communicates with Master and Slave2
*
* <SSI port usage>
* Slave2 - Master:	SSI3 (PD0-ssi3clk, PD1-ssi3fss, PD2-ssi3RX, PD3-ssi3TX)
* Slave2 - Slave1:	SSI1 (PF2-ssi1clk, PF3-ssi1fss, PF0-ssi1RX, PF1-ssi1TX)
*
* Credit : Zee Lv
*/


#include <stdint.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "slave2_ssi.h"


/*
 *  SSI3 D Conncection
 *  ------------------
 *  SCK  --------- PD0
 *  SS   --------- PD1
 *  MISO --------- PD2
 *  MOSI --------- PD3
 *
 *  SSI1 F Conncection
 *  ------------------
 *  SCK  --------- PF2
 *  SS   --------- PF3
 *  MISO --------- PF0
 *  MOSI --------- PF1
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
static uint8_t reverseBitOrder(uint8_t byte) {
    return (((byte & 0x01) << 7) +
            ((byte & 0x02) << 5) +
            ((byte & 0x04) << 3) +
            ((byte & 0x08) << 1) +
            ((byte & 0x10) >> 1) +
            ((byte & 0x20) >> 3) +
            ((byte & 0x40) >> 5) +
            ((byte & 0x80) >> 7));
}


/****************************************************
 *                                                  *
 *                   SS Functions                   *
 *                                                  *
 ****************************************************/

static void SSI3_SS_HIGH (void) {
    GPIO_PORTD_DATA_R |= 0x02;
}

static void SSI3_SS_LOW (void) {
    GPIO_PORTD_DATA_R &= ~0x02;
}

static void SS1_SS_HIGH (void) {
    GPIO_PORTF_DATA_R |= 0x08;
}

static void SS1_SS_LOW (void) {
    GPIO_PORTF_DATA_R &= ~0x08;
}


/****************************************************
 *                                                  *
 *                   Initializer                    *
 *                                                  *
 ****************************************************/

/**
 * Slave2_SSI_Init
 * SSI3 and SSI1 init
 */
void Slave2_SSI_Init(void)
{
	SSI3_Init();
	SSI1_Init();
}

/**
 * SSI3_Init
 * Communication with the master
 * Slave configuration
 */
void SSI3_Init(void) {
    /* SSI3 and Port D Activation */
    SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R3;                 // enable SSI Module 3 clock
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;               // enable GPIO Port D clock
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R3) == 0) {};  // allow time for activating
    
    /* Port D Set Up */
    GPIO_PORTD_DIR_R |= 0x02;                              // make PD1 output
    GPIO_PORTD_AFSEL_R |= 0x0D;                            // enable alt funct on PD0, 2, 3
    GPIO_PORTD_AFSEL_R &= ~0x02;                           // disable alt funct on PD1
    GPIO_PORTD_PCTL_R &= ((~GPIO_PCTL_PD0_M) &             // clear bit fields for PD0
                          (~GPIO_PCTL_PD1_M) &             // clear bit fields for PD1, PD1 will be used as GPIO
                          (~GPIO_PCTL_PD2_M) &             // clear bit fields for PD2
                          (~GPIO_PCTL_PD3_M));             // clear bit fields for PD3
    GPIO_PORTD_PCTL_R |= (GPIO_PCTL_PD0_SSI3CLK |          // configure PD0 as SSI3CLK
                          GPIO_PCTL_PD2_SSI3RX  |          // configure PD2 as SSI3RX
                          GPIO_PCTL_PD3_SSI3TX);           // configure PD3 as SSI3TX
    GPIO_PORTD_AMSEL_R &= ~0x0F;                           // disable analog functionality on PD0-3
    GPIO_PORTD_DEN_R |= 0x0F;                              // enable digital I/O on PD0-3
    
    /* SSI3 Set Up */
    SSI3_CR1_R &= ~SSI_CR1_SSE;                            // disable SSI operation
    SSI3_CR1_R |= SSI_CR1_MS;                             // configure SSI3 as slave mode
    SSI3_CPSR_R &= SSI_CPSR_CPSDVSR_M;                     // clear bit fields for SSI Clock Prescale Divisor
    SSI3_CPSR_R += 40;                                     // /40 clock divisor, 2Mhz
    SSI3_CR0_R &= ~SSI_CR0_SCR_M;                          // clear bit fields for SSI3 Serial Clock Rate, SCR = 0
    SSI3_CR0_R &= ~SSI_CR0_SPO;                            // clear bit fields for SSI3 Serial Clock Polarity, SPO = 0
    SSI3_CR0_R &= ~SSI_CR0_SPH;                            // clear bit fields for SSI3 Serial Clock Phase, SPH = 0
    SSI3_CR0_R &= ~SSI_CR0_FRF_M;                          // clear bit fields for SSI3 Frame Format Select
    SSI3_CR0_R |= SSI_CR0_FRF_MOTO;                        // set frame format to Freescale SPI Frame Format
    SSI3_CR0_R &= ~SSI_CR0_DSS_M;                          // clear bit fields for SSI3 Data Size Select
    SSI3_CR0_R |= SSI_CR0_DSS_8;                           // set SSI data size to 8
    SSI3_CR1_R |= SSI_CR1_SSE;                             // enable SSI operation
}

/**
 * SSI1_Init
 * Communication with the slave1
 * Slave configuration
 */
void SSI1_Init(void) {
    /* SSI1 Activation */
    SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R1;                 // enable SSI Module 1 clock
    
    /* Port F Activation */
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;               // enable GPIO Port F clock
    while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0) {};  // allow time for activating
    
    /* Port F Set Up */
    GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;                     // unlock GPIO Port F --- this step is only for Port F
    GPIO_PORTF_CR_R = 0x0F;                                // allow changes to PF0-3 --- this step is only for Port F
    GPIO_PORTF_DIR_R |= 0x08;                              // make PF3 output
    GPIO_PORTF_AFSEL_R |= 0x07;                            // enable alt funct on PF0-2
    GPIO_PORTF_AFSEL_R &= ~0x08;                           // disable alt funct on PF3
    GPIO_PORTF_PCTL_R &= ((~GPIO_PCTL_PF0_M) &             // clear bit fields for PF0
                          (~GPIO_PCTL_PF1_M) &             // clear bit fields for PF1
                          (~GPIO_PCTL_PF2_M) &             // clear bit fields for PF2
                          (~GPIO_PCTL_PF3_M));             // clear bit fields for PF3, PF3 will be used as GPIO
    GPIO_PORTF_PCTL_R |= (GPIO_PCTL_PF0_SSI1RX  |          // configure PF0 as SSI1RX
                          GPIO_PCTL_PF1_SSI1TX  |          // configure PF1 as SSI1TX
                          GPIO_PCTL_PF2_SSI1CLK);          // configure PF2 as SSI1CLK
    GPIO_PORTF_AMSEL_R &= ~0x0F;                           // disable analog functionality on PF0-3
    GPIO_PORTF_DEN_R |= 0x0F;                              // enable digital I/O on PF0-3
    
    
    /* SSI1 Set Up */
    SSI1_CR1_R &= ~SSI_CR1_SSE;                            // disable SSI operation
    SSI1_CR1_R |= SSI_CR1_MS;                             // configure SSI1 as slave mode
    SSI1_CPSR_R &= SSI_CPSR_CPSDVSR_M;                     // clear bit fields for SSI Clock Prescale Divisor
    SSI1_CPSR_R += 40;                                     // /40 clock divisor, 2Mhz
    SSI1_CR0_R &= ~SSI_CR0_SCR_M;                          // clear bit fields for SSI1 Serial Clock Rate, SCR = 0
    SSI1_CR0_R &= ~SSI_CR0_SPO;                            // clear bit fields for SSI1 Serial Clock Polarity, SPO = 0
    SSI1_CR0_R &= ~SSI_CR0_SPH;                            // clear bit fields for SSI1 Serial Clock Phase, SPH = 0
    SSI1_CR0_R &= ~SSI_CR0_FRF_M;                          // clear bit fields for SSI1 Frame Format Select
    SSI1_CR0_R |= SSI_CR0_FRF_MOTO;                        // set frame format to Freescale SPI Frame Format
    SSI1_CR0_R &= ~SSI_CR0_DSS_M;                          // clear bit fields for SSI1 Data Size Select
    SSI1_CR0_R |= SSI_CR0_DSS_8;                           // set SSI data size to 8
    SSI1_CR1_R |= SSI_CR1_SSE;                             // enable SSI operation
}


/****************************************************
 *                                                  *
 *                   I/O Functions                  *
 *                                                  *
 ****************************************************/

/**
 * SSI3_read
 * ----------
 * @return date read from master 
 */
uint16_t SSI3_read (void) {
    while((SSI3_SR_R & SSI_SR_BSY) == SSI_SR_BSY) {};     // wait until SSI3 not busy/transmit FIFO empty
    SSI3_DR_R = 0x00;                                     // data out, garbage, just for synchronization
    while((SSI3_SR_R & SSI_SR_RNE) == 0) {};              // wait until response
    return SSI3_DR_R;                                     // read data
    
}

/**
 * SSI3_write
 * ----------
 * @param  data  data to be written.
 */
void SSI3_write (uint16_t data){
    while ((SSI3_SR_R & SSI_SR_BSY) == SSI_SR_BSY) {};   // wait until SSI3 not busy/transmit FIFO empty
    SSI3_DR_R = data;                                    // write data
    while ((SSI3_SR_R & SSI_SR_RNE) == 0) {};            // wait until response
    uint16_t sync = SSI3_DR_R;                           // read byte of data, just for synchronization
}

/**
 * SSI1_read
 * ----------
 * @return date read from slave1
 */
uint16_t SSI1_read (void) {
    while((SSI1_SR_R & SSI_SR_BSY) == SSI_SR_BSY) {};     // wait until SSI1 not busy/transmit FIFO empty
    SSI1_DR_R = 0x00;                                     // data out, garbage, just for synchronization
    while((SSI1_SR_R & SSI_SR_RNE) == 0) {};              // wait until response
    return SSI1_DR_R;                                     // read data
}

/**
 * SSI1_write
 * ----------
 * @param  data  data to be written.
 */
void SSI1_write(uint16_t data){
    while ((SSI1_SR_R & SSI_SR_BSY) == SSI_SR_BSY) {};   // wait until SSI3 not busy/transmit FIFO empty
    SSI1_DR_R = data;                                    // write data
    while ((SSI1_SR_R & SSI_SR_RNE) == 0) {};            // wait until response
    uint16_t sync = SSI1_DR_R;                           // read byte of data, just for synchronization
}


