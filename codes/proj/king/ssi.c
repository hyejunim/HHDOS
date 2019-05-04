#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "OS.h"


#define SDC_CS_PB0 1
#if SDC_CS_PB0
// CS is PB0
// to change CS to another GPIO, change SDC_CS and CS_Init
#define SDC_CS   (*((volatile unsigned long *)0x40005004)) 
#define SDC_CS_LOW       0           // CS controlled by software
#define SDC_CS_HIGH      0x01  
#endif

#define TFT_CS                  (*((volatile unsigned long *)0x40004020))
#define TFT_CS_LOW              0           // CS normally controlled by hardware
#define TFT_CS_HIGH             0x08
#define DC                      (*((volatile unsigned long *)0x40004100))
#define DC_COMMAND              0
#define DC_DATA                 0x40
#define RESET                   (*((volatile unsigned long *)0x40004200))
#define RESET_LOW               0
#define RESET_HIGH              0x80
#define SSI_CR0_SCR_M           0x0000FF00  // SSI Serial Clock Rate
#define SSI_CR0_SPH             0x00000080  // SSI Serial Clock Phase
#define SSI_CR0_SPO             0x00000040  // SSI Serial Clock Polarity
#define SSI_CR0_FRF_M           0x00000030  // SSI Frame Format Select
#define SSI_CR0_FRF_MOTO        0x00000000  // Freescale SPI Frame Format
#define SSI_CR0_DSS_M           0x0000000F  // SSI Data Size Select
#define SSI_CR0_DSS_8           0x00000007  // 8-bit data
#define SSI_CR1_MS              0x00000004  // SSI Master/Slave Select
#define SSI_CR1_SSE             0x00000002  // SSI Synchronous Serial Port
                                            // Enable
#define SSI_SR_BSY              0x00000010  // SSI Busy Bit
#define SSI_SR_TNF              0x00000002  // SSI Transmit FIFO Not Full
#define SSI_CPSR_CPSDVSR_M      0x000000FF  // SSI Clock Prescale Divisor
#define SSI_CC_CS_M             0x0000000F  // SSI Baud Clock Source
#define SSI_CC_CS_SYSPLL        0x00000000  // Either the system clock (if the
                                            // PLL bypass is in effect) or the
                                            // PLL output (default)
#define SYSCTL_RCGC1_SSI0       0x00000010  // SSI0 Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control


/*
void static writecommand(unsigned char c) {
  volatile uint32_t response;
                                        // wait until SSI0 not busy/transmit FIFO empty
  while((SSI0_SR_R&SSI_SR_BSY)==SSI_SR_BSY){};
  SDC_CS = SDC_CS_HIGH;
  TFT_CS = TFT_CS_LOW;
  DC = DC_COMMAND;
  SSI0_DR_R = c;                        // data out
  while((SSI0_SR_R&SSI_SR_RNE)==0){};   // wait until response
  TFT_CS = TFT_CS_HIGH;
  response = SSI0_DR_R;                 // acknowledge response
}

void static writedata(unsigned char c) {
  volatile uint32_t response;
                                        // wait until SSI0 not busy/transmit FIFO empty
  while((SSI0_SR_R&SSI_SR_BSY)==SSI_SR_BSY){};
  SDC_CS = SDC_CS_HIGH;
  TFT_CS = TFT_CS_LOW;
  DC = DC_DATA;
  SSI0_DR_R = c;                        // data out
  while((SSI0_SR_R&SSI_SR_RNE)==0){};   // wait until response
  TFT_CS = TFT_CS_HIGH;
  response = SSI0_DR_R;                 // acknowledge response
}

// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in ROM byte array.
void static commandList(const unsigned char *addr) {

  unsigned char numCommands, numArgs;
  unsigned short ms;

  numCommands = *(addr++);               // Number of commands to follow
  while(numCommands--) {                 // For each command...
    writecommand(*(addr++));             //   Read, issue command
    numArgs  = *(addr++);                //   Number of args to follow
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--) {                   //   For each argument...
      writedata(*(addr++));              //     Read, issue argument
    }

    if(ms) {
      ms = *(addr++);             // Read post-command delay time (ms)
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      Delay1ms(ms);
    }
  }
}

// Initialization code common to both 'B' and 'R' type displays
void static commonInit(const unsigned char *cmdList) {

  SYSCTL_RCGCSSI_R |= 0x01;  // activate SSI0
  SYSCTL_RCGCGPIO_R |= 0x01; // activate port A
  while((SYSCTL_PRGPIO_R&0x01)==0){};

  // toggle RST low to reset; CS low so it'll listen to us
  // SSI0Fss is temporarily used as GPIO
  GPIO_PORTA_DIR_R |= 0xC8;             // make PA3,6,7 out
  GPIO_PORTA_AFSEL_R &= ~0xC8;          // disable alt funct on PA3,6,7
  GPIO_PORTA_DEN_R |= 0xC8;             // enable digital I/O on PA3,6,7
                                        // configure PA3,6,7 as GPIO
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0x00FF0FFF)+0x00000000;
  GPIO_PORTA_AMSEL_R &= ~0xC8;          // disable analog functionality on PA3,6,7
  TFT_CS = TFT_CS_LOW;
  RESET = RESET_HIGH;
  Delay1ms(500);
  RESET = RESET_LOW;
  Delay1ms(500);
  RESET = RESET_HIGH;
  Delay1ms(500);

  // initialize SSI0
  GPIO_PORTA_AFSEL_R |= 0x34;           // enable alt funct on PA2,4,5
  GPIO_PORTA_DEN_R |= 0x34;             // enable digital I/O on PA2,4,5
                                        // configure PA2,4,5 as SSI
  GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R&0xFF0F00FF)+0x00202200;
  GPIO_PORTA_AMSEL_R &= ~0x34;          // disable analog functionality on PA2,4,5
  SSI0_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
  SSI0_CR1_R &= ~SSI_CR1_MS;            // master mode
                                        // clock divider for 10 MHz SSIClk (assumes 80 MHz PIOSC)
  SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+8; 
  // CPSDVSR must be even from 2 to 254
  
  SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (80 Mbps base clock)
                  SSI_CR0_SPH |         // SPH = 0
                  SSI_CR0_SPO);         // SPO = 0
                                        // FRF = Freescale format
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                        // DSS = 8-bit data
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
  SSI0_CR1_R |= SSI_CR1_SSE;            // enable SSI

  if(cmdList) commandList(cmdList);
}
//------------ST7735_InitR------------
// Initialization for ST7735R screens (green or red tabs).
// Input: option one of the enumerated options depending on tabs
// Output: none
void ST7735_InitR(enum initRFlags option) {
  commonInit(Rcmd1);
  if(option == INITR_GREENTAB) {
    commandList(Rcmd2green);
    ColStart = 2;
    RowStart = 1;
  } else {
    // colstart, rowstart left at default '0' values
    commandList(Rcmd2red);
  }
  commandList(Rcmd3);

  // if black, change MADCTL color filter
  if (option == INITR_BLACKTAB) {
    writecommand(ST7735_MADCTL);
    writedata(0xC0);
  }
  TabColor = option;
  OS_InitSemaphore(&LCDFree,1);  // means LCD free

}
*/


void static writecommand(unsigned char c) {
  volatile uint32_t response;
                                        // wait until SSI0 not busy/transmit FIFO empty
  while((SSI2_SR_R&SSI_SR_BSY)==SSI_SR_BSY){};
  TFT_CS = TFT_CS_LOW;
  DC = DC_COMMAND;
  SSI2_DR_R = c;                        // data out
  while((SSI2_SR_R&SSI_SR_RNE)==0){};   // wait until response
  TFT_CS = TFT_CS_HIGH;
  response = SSI2_DR_R;                 // acknowledge response
}

void static writedata(unsigned char c) {
  volatile uint32_t response;
                                        // wait until SSI0 not busy/transmit FIFO empty
  while((SSI2_SR_R&SSI_SR_BSY)==SSI_SR_BSY){};
  TFT_CS = TFT_CS_LOW;
  DC = DC_DATA;
  SSI2_DR_R = c;                        // data out
  while((SSI2_SR_R&SSI_SR_RNE)==0){};   // wait until response
  TFT_CS = TFT_CS_HIGH;
  response = SSI2_DR_R;                 // acknowledge response
}

//------------SSI2_M_Init------------
// Initialization for SSI2 Master
// PB4,5,6,7(clk, fss, rx, tx)
// Input: none
// Output: none
void SSI2_M_Init() {
  SYSCTL_RCGCSSI_R |= 0x04;  // activate SSI2
  SYSCTL_RCGCGPIO_R |= 0x02; // activate port B
  while((SYSCTL_PRGPIO_R&0x02)==0){};
		
  // initialize SSI0
  GPIO_PORTB_AFSEL_R |= 0x34;           // enable alt funct on PB4,5,6,7
  GPIO_PORTB_DEN_R |= 0x34;             // enable digital I/O on PB4,5,6,7
                                        // configure PB4,5,6,7 as SSI
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0x0000FFFF)+0x22220000;
  GPIO_PORTB_AMSEL_R &= ~0xF0;          // disable analog functionality on PB4,5,6,7
  SSI2_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
  SSI2_CR1_R &= ~SSI_CR1_MS;            // master mode
                                        // clock divider for 10 MHz SSIClk (assumes 80 MHz PIOSC)
  SSI2_CPSR_R = (SSI2_CPSR_R&~SSI_CPSR_CPSDVSR_M)+8; 
  // CPSDVSR must be even from 2 to 254
  
  SSI2_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (80 Mbps base clock)
                  SSI_CR0_SPH |         // SPH = 0
                  SSI_CR0_SPO);         // SPO = 0
                                        // FRF = Freescale format
  SSI2_CR0_R = (SSI2_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                        // DSS = 8-bit data
  SSI2_CR0_R = (SSI2_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
  SSI2_CR1_R |= SSI_CR1_SSE;            // enable SSI

/*
  // toggle RST low to reset; CS low so it'll listen to us
  // SSI0Fss is temporarily used as GPIO
  GPIO_PORTB_DIR_R |= 0x08;             // make PB3 out
  GPIO_PORTB_AFSEL_R &= ~0x08;          // disable alt funct on PB3
  GPIO_PORTB_DEN_R |= 0x08;             // enable digital I/O on PB3
                                        // configure PB3 as GPIO
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF0FFF)+0x00000000;
  GPIO_PORTB_AMSEL_R &= ~0x08;          // disable analog functionality on PB3
  TFT_CS = TFT_CS_LOW;
  RESET = RESET_HIGH;
  Delay1ms(500);
  RESET = RESET_LOW;
  Delay1ms(500);
  RESET = RESET_HIGH;
  Delay1ms(500);
  // initialize SSI0
  GPIO_PORTB_AFSEL_R |= 0x34;           // enable alt funct on PB2,4,5
  GPIO_PORTB_DEN_R |= 0x34;             // enable digital I/O on PB2,4,5
                                        // configure PB2,4,5 as SSI
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFF0F00FF)+0x00202200;
  GPIO_PORTB_AMSEL_R &= ~0x34;          // disable analog functionality on PB2,4,5
  SSI0_CR1_R &= ~SSI_CR1_SSE;           // disable SSI
  SSI0_CR1_R &= ~SSI_CR1_MS;            // master mode
                                        // clock divider for 10 MHz SSIClk (assumes 80 MHz PIOSC)
  SSI0_CPSR_R = (SSI0_CPSR_R&~SSI_CPSR_CPSDVSR_M)+8; 
  // CPSDVSR must be even from 2 to 254
  
  SSI0_CR0_R &= ~(SSI_CR0_SCR_M |       // SCR = 0 (80 Mbps base clock)
                  SSI_CR0_SPH |         // SPH = 0
                  SSI_CR0_SPO);         // SPO = 0
                                        // FRF = Freescale format
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_FRF_M)+SSI_CR0_FRF_MOTO;
                                        // DSS = 8-bit data
  SSI0_CR0_R = (SSI0_CR0_R&~SSI_CR0_DSS_M)+SSI_CR0_DSS_8;
  SSI0_CR1_R |= SSI_CR1_SSE;            // enable SSI

*/
}
