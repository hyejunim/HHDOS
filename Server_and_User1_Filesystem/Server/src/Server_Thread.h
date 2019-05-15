
#include <stdint.h>


void Thread_Initialize(void);
//******** Record_Init  *************** 
void Record_Init(void);




////////////////general send/receive for all ///////////////////////////

//******** Thread_Rcv1CAN  *************** 
void Thread_Rcv1CAN(void);

//******** Server_Rcv1Data_Task  *************** 
void Server_Rcv1Data_Task(void);

//******** Server_Xmt1Data_Task  *************** 
void Server_Xmt1Data_Task(void);



///////////////////receiving only for wq////////////////////////////
//******** Rcv1_WQ_Task  *************** 
void Rcv1_WQ_Task(void);

//******** Thread_Rcv1_WQ  *************** 
void Thread_Rcv1_WQ(void);





//******** IdleTask  *************** 
void IdleTask(void);

//******** PThread_XtmCAN  *************** 
void PThread_XtmCAN(void);


