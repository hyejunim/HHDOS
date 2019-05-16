
#define LIST 0
#define READ 1
#define WRITE 2
#define REMOVE 3
#define FORMAT 4
#define WQ 5 // :wq

extern uint8_t wqfilename[7];
extern uint8_t wqoriginal[1024];
extern uint8_t wqmodified[1024];
extern uint16_t wqfilesize;

void UART_OutCRLF(void);

//------------interpreter----------
void Interpreter(void);
void Interpreter_Init(void);


