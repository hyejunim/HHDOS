#define MAX_ARRSIZE 256

//extern unsigned long arr_size;
extern uint16_t arr_size;
extern uint8_t arr[MAX_ARRSIZE];


void create_testArr(void);




//******** MergeSort  *************** 
// Iterative merge sort function of an array number of size
void MergeSort(uint8_t* pArr, uint8_t size);

void Thread_Merge(void);
void Merge(uint8_t arr[], int l, int m, int r);

void IdleTask(void);
