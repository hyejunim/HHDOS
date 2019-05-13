

//******** Thread_CANGetArrSize  *************** 
// Get Array Size from CAN
// Kills itself after receiving
void Thread_CANGetArrSize(void);

//******** Thread_CANGetArrSize  *************** 
// Get Array data from CAN and store it into the local array in the right place
// Kills itself after receiving
void Thread_CANGetArr8_Sort(void);

//******** Thread_CANSendArr  *************** 
// Send Array to CAN
// Kills itself after sending
void Thread_CANSendArr(void);
