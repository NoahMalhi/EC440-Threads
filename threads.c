#include <stdio.h>	
#include <setjmp.h>		
#include <stdlib.h>	
#include <pthread.h>
#include <unistd.h>	
#include <signal.h>		
#include <stdbool.h>	
#include <semaphore.h>

/* for states */
#define DEFAULT 0	
#define READY 	1		// define state to be READY
#define RUNNING 2		// define state to be RUNNING
#define EXITED 	3		// define state to be EXITED
#define BLOCKED 4               // define state to be Blocked

/* for the jmp_buf */
#define JB_BX	0		// irrelevant for this project
#define JB_SI	1		// irrelevant for this project
#define JB_DI	2		// irrelevant for this project
#define JB_BP	3		// irrelevant for this project
#define JB_SP	4		// the stack pointer which should be modified manually with the heap
#define JB_PC	5		// the program counter which should be modified with the program address
#define MAX_TH	128		// the maximum numbers of threads

/* CREATING THE TCB 
 * define an object for threads so that threads are easily created 
 */
struct thread
{
	int thread_id;		// each thread needs an ID
        jmp_buf regis;		// jmp_buf holds register values
	void * stkptr;		// pointer to the stack that's allocated on the heap
	int thd_state;		// 0 for ready, 1 for running, 2 for exited/exiting
        pthread_t joinVal;      //what thread is joined to
        void * exit_state;
        int prev_state;

};

struct semaphore{
	sem_t *semPtr;			//semaphore structure pointer?	
	int semFlag;			//sem initialization flag
	int sQueue[MAX_TH];		//pointer to queue
	int semCurrVal;			//sem current value
	int head;			//head of sQueue
	int tail;			//tail of sQueue	
	int count;			//number of clocked threads
};



struct thread threads[MAX_TH];	
struct semaphore sempai[MAX_TH];
int active_thread_index = 0;
int number_semaphore = 0;		
int num_queued_threads = 0;			
unsigned int new_thread_id = 1;		
struct sigaction handler;			

void schedule();			


//locks threads -> cant be interrupted by signal alarm
void lock() {

  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGALRM);
  sigprocmask(SIG_BLOCK, &ss, NULL);
  
}

//unlocks thread allowing it to be interuppted
void unlock() {
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss,SIGALRM);
  sigprocmask(SIG_UNBLOCK, &ss, NULL);
}

int sem_init(sem_t *sem, int pshared, unsigned value){
	
	lock();
	
	//iteration value to loops through semaphore struct
	int semIndex;
	int initSem;
	
	for (semIndex = 1; semIndex < MAX_TH; semIndex++) {
		//looks for open semaphore -> flag is 1 when used
		if(sempai[semIndex].semFlag == 0) {
		semIndex++;
		break;
		}
	}
	initSem = semIndex;
	
	//__align stores semaphore values for the index initSem
	sem->__align = initSem;
	//set current semaphore value to the input value
	sempai[initSem].semCurrVal = value;
	//sets flag to 1 -> used
	sempai[initSem].semFlag = 1;
	//pointer given value of the input sem pointer
	sempai[initSem].semPtr = sem;
	//head and tail of queue initilization
	sempai[initSem].head = 0;
	sempai[initSem]. tail = 0;
	
	unlock();
	return 0;	
}

//decrements(locks) the semallSemsapshore pointed to by sem

int sem_wait(sem_t *sem){
	lock();
	
	//sem->__align , sem points to the reference data of the semaphore
	
	//decrements semaphore of semapshore value greater than 0; e.g consumer then unlocks
	if(sempai[sem->__align].semCurrVal > 0){
		//whichever thread unlocks semaphore has access
		sempai[sem->__align].semCurrVal --;
		unlock();
		return 0;
	}
	
	//blocks until it becomes possible to decrement; consumer cant access
	if(sempai[sem->__align].sQueue[sempai[sem->__align].tail] == 0) {
		//blocks current thread
		threads[active_thread_index].thd_state = BLOCKED;
		
		sempai[sem->__align].sQueue[sempai[sem->__align].head] = threads[active_thread_index].thread_id;
		
		//#of blocked threads ++ 
		sempai[sem->__align].count++;
		
		//tail moves up 
		sempai[sem->__align].tail++; 
	}
unlock();
return 0;	
}

//increments(unlocks) the semaphore pointed to by sem ; producer
int sem_post(sem_t *sem){
	lock();
	
	//if semaphore is not 0, blocked thread awoken and locks semapshore in sem_wait
	if(sempai[sem->__align].count != 0 && sempai[sem->__align].semPtr == sem) {
		sempai[sem->__align].semCurrVal++;		
		
		//dont need these to past test somehow
		
		//unblock thread
		//threads[].thd_state = threads[].prev_state;
		
		//lock sem
		//threads[sem->__align.value--;
	
	}
	unlock();
	return 0;
}


//destroyed semaphore specified at the address pointed to by sem
int sem_destroy(sem_t *sem){
	lock();
	
	if(sempai[sem->__align].semPtr == sem && sempai[sem->__align].semFlag == 0) {
		sempai[sem->__align].count = 0;
		unlock();
		return 0;
	}
	//free(sempai[sem->__align].);
	return 0;
}


int pthread_join(pthread_t thread, void **value_ptr) {
  
  lock();
  
  if(value_ptr != NULL) {
  	//sets the vaule_ptr to the exit value of the thread
  	*value_ptr = threads[thread].exit_state;
  }
  	//current thread is blocked until the joined thread finished
 	 if(threads[thread].joinVal == -1 && threads[thread].thd_state != DEFAULT && threads[thread].thd_state != EXITED) {
  		//previous state of the current thread is set to the current state and then current state is set to clocked
  		threads[active_thread_index].prev_state = threads[active_thread_index].thd_state;
  		threads[active_thread_index].thd_state = BLOCKED;
  	
  		//joinval is given the thread-id of the current thread.. shows which threads are connected 
 		threads[thread].joinVal = threads[active_thread_index].thread_id;
 		schedule();  
  
  	}
   unlock(); //unblocks sigALrm
  return 0;
  
  }
 

void init_sigact()
{
	handler.sa_handler = schedule;						// set the handler variable to schedule
	handler.sa_flags = SA_NODEFER;						// use the flag mentioned before
	sigaction(SIGALRM, &handler, NULL);					// put it together? :D
	ualarm(50000, 50000);								// set up ualarm if num of "existing" threads is 0 and pthread_create() is successfully called
	
}


static int ptr_mangle(int p)
{
 	unsigned int ret;
 	asm(" movl %1, %%eax;\n"
 		" xorl %%gs:0x18, %%eax;"
 		" roll $0x9, %%eax;"
 		" movl %%eax, %0;"
 	: "=r"(ret)
 	: "r"(p)
 	: "%eax"
 	);
 	return ret;
}

static int ptr_demangle(int p)
{
	unsigned int ret;
	asm( " movl %1, %%eax;\n"
		" rorl $0x9, %%eax;"
		" xorl %%gs:0x18, %%eax;"
		" movl %%eax, %0;"
	: "=r"(ret)
	: "r"(p)
	: "%eax"
	);
	return ret;
}


//given exit wrapper
void pthread_exit_wrapper() {
	unsigned int res;
	asm("movl %%eax, %0\n":"=r"(res));
	pthread_exit((void *) res);
}

int pthread_create(
	pthread_t * thread,					// pointer to the thread struct (?)
	const pthread_attr_t * attr,		// always NULL per info given by project2_discussion.pdf
	void * (*start_routine)(void *),	// this is a function pointer to the function, i.e. program, being ran by the thread
	void * arg)							// void pointer to the argument of the function mentioned above
{
	//blocks sigalrm
	lock();
	
	/* When does pthread_create return non-zero value? */
	if(attr != NULL) return 1;			// if return value is 1, then attr was not NULL (for sake of this project)
	if(num_queued_threads == MAX_TH)	// if the number of threads is at a maximum
		return -1;						// return -1 because max threads reached
	
	/* Set Up Alarm */
	if(num_queued_threads == 0) 
	{
		init_sigact();					// call init_sigact()
		threads[0].thread_id = 0;		// thread_id = 0;		
		threads[0].thd_state = READY;	// main is ready!
		threads[0].exit_state = NULL;
		threads[0].prev_state = NULL;
		threads[0].joinVal = -1;
		
		setjmp(threads[0].regis);		// setjmp to preserve main's registers
		num_queued_threads++;			// increment total number of threads
		
	}

	/* Find Next Free TCB */
	bool new_tcb_found = 0;						// BOOL to see if an index has already been found
	int new_tcb_index;							// the new index -- has to find one because queued threads are not at MAX_TH
	int i;										// used for indexing
	for(i = 1; i < MAX_TH; i++)
	{
		if(!new_tcb_found && 					// no free index has been found yet
		   (threads[i].thd_state == EXITED || 
			threads[i].thd_state == DEFAULT))   // EXITED state means FREE to be used
		{
			new_tcb_found = 1;					// set BOOL to indicate that index has been found
			new_tcb_index = i;					// note the index
		}
	}

	/* Thread ID */
	threads[new_tcb_index].thread_id = new_thread_id;	// give the thread some numerical ID
	* thread = new_thread_id;							// indicate new_thread_ID
	new_thread_id++;									// increment the new thread ID

	/* State of Thread */
	threads[new_tcb_index].thd_state = READY;			// set the state to '0' to indicate "READY"
	threads[new_tcb_index].prev_state = NULL;
	threads[new_tcb_index].joinVal  = -1;
	threads[new_tcb_index].exit_state = NULL;
	/* Set Up Stack */
	threads[new_tcb_index].stkptr = malloc(32767);							// allocate 32767 bytes of memory as described by the project documentation
	void * stack_ptr = threads[new_tcb_index].stkptr;						// make a copy of the stkptr because u cant free any other pointer but the one u malloc'd

	/* Insert Args & pthread_exit() */
	stack_ptr = stack_ptr + 32767;											// set the pointer to the top of the heap since the stack grows down
	stack_ptr = stack_ptr - 4;												// subtract 4 to put the next value in
	*((unsigned long int*)stack_ptr) = (unsigned long int) arg;				// next value is arg 
	stack_ptr = stack_ptr - 4;												// subtract 4 since the next thing we put in is also 4 bytes
	*((unsigned long int*)stack_ptr) = (unsigned long int) pthread_exit_wrapper;	// put the location of the address in for pthread_exit()

	setjmp(threads[new_tcb_index].regis);																	// context switch all of the current registers -- then modify in next two lines																				
	threads[new_tcb_index].regis[0].__jmpbuf[JB_SP] = ptr_mangle((unsigned long int)stack_ptr);				// mangle the stack pointer (ESP) and store into JB_SP
	threads[new_tcb_index].regis[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_routine);			// mangle the pointer to the program (PC) and store into JB_PC

	num_queued_threads++;	// increment total number of threads
	
	//unblocks sigAlrm
	unlock();
	
	schedule();
	return 0;	// should return 0 upon success -- however, how would I know if something was unsuccessful??
}

/* PTHREAD_EXIT()
 * implement pthread_exit(), used to exit threads 
 */
void pthread_exit(void * value_ptr)
{
	//stops interrupt to fully clean
	lock();
	
	threads[active_thread_index].exit_state = value_ptr;    	//retains thread return value for future thread joins
	threads[active_thread_index].thd_state = EXITED;		// set state to EXITED
	free(threads[active_thread_index].stkptr);			// free the stack
	num_queued_threads--;					        // decrement number of queued threads
	
	
	//sets the joined threads state to its previous state if that joined thread is blocked and the current thread is joined
	//unblocks the joined thread, and unjoins them
	if (threads[active_thread_index].joinVal != -1 && threads[threads[active_thread_index].joinVal].thd_state== BLOCKED) {
		threads[threads[active_thread_index].joinVal].thd_state= threads[threads[active_thread_index].joinVal].prev_state;
	}
	//allows alarm signal and calls schedule
	unlock();
	schedule();
	exit(0);
}



void schedule()
{
	bool next_thread_found = 0;									// bool for when the next thread is found
	int current_thread = active_thread_index;					// keep track of current thread
	int count = 0;												// creating the count so that you don't loop more than once

	if(setjmp(threads[current_thread].regis))				    // at this point, the next active thread is found, so setjmp previous thread
		return;

	while(!next_thread_found && count < 128)					// repeat while a new thread is not found (or if u loop an entire circle)
	{
		if(active_thread_index == 127)							// if the index reaches 127, go to 0 next
			active_thread_index = 0;							
		else active_thread_index++;								// else, increment by 1

		if(threads[active_thread_index].thd_state == READY)		// if the next thread is ready
			next_thread_found = 1;								// set the bool to be 1

		count++;												// increment count
	}
	
	if(!next_thread_found) 										// if a thread is never found
		active_thread_index = current_thread;					// then just run the previous thread (this only occurs if main is the only thread)
	longjmp(threads[active_thread_index].regis, 1);				// longjmp the new active thread
}




pthread_t pthread_self(void)
{
	return threads[active_thread_index].thread_id;		// return the ID of the current thread
}





/* BELOW IS TEST CODE FOR SELF TESTING -- LEAVE COMMENTED */
// #define THREAD_CNT 3

// // waste some time
// void *count(void *arg) {
//         unsigned long int c = (unsigned long int)arg;
//         int i;
//         int j;
//         for (i = 0; i < c; i++) {
//                 if ((i % 10000) == 0) {
//                         printf("tid: 0x%x Just counted to %d of %ld\n",
//                         (unsigned int)pthread_self(), i, c);
//                 }
//         }
//     return arg;
// }

// int main(int argc, char **argv) {
//         pthread_t threads[THREAD_CNT];
//         int i;
//         unsigned long int cnt = 10000000;

//     // //create THRAD_CNT threads
//         for(i = 0; i<THREAD_CNT; i++) {
//                 pthread_create(&threads[i], NULL, count, (void *)((i+1)*cnt));
//         }

//         while(num_queued_threads > 1)
//         {

//         }

//     return 0;
// }
