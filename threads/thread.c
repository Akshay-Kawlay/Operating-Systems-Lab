#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

void
thread_stub(void (*thread_main)(void *), void *arg);

/*keeping track of current running thread*/
Tid current_thread_id = 0;
void *stack_mem_to_be_deleted = NULL ;
struct ready_queue readyQ;

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	int isValid;
	Tid index;
	struct ucontext thread_context;
	void *stack_memory;
	struct thread *next;
};
/*global array of Threads: TCB, static*/
struct thread TCB [THREAD_MAX_THREADS];

struct ready_queue{
	struct thread *head;
	struct thread *tail;
};
/*
struct exit_queue{
	struct exit_queue_node *head;
};

struct exit_queue_node{
	gregs_t *thread_stack;
	struct exit_queue_node *next;
};

struct exit_queue exitQ;
*/

void
thread_init(void)
{
	/* your optional code here */

	readyQ.head = NULL;
	readyQ.tail = NULL;
	/*initialise current thread (1st thread) & stub function*/
	current_thread_id = 0;
	TCB[0].isValid = 1;
	TCB[0].index = 0;
	int i;
	for(i = 1; i<THREAD_MAX_THREADS; i++){
		TCB[i].isValid = 0;
	//	TCB[i].index = i;
	}
	
}

Tid
thread_id(){	
	return current_thread_id;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	/*acquire a spot in TCB for the new thread*/
	int i = 0;
	while(TCB[i].isValid){
		if(i == THREAD_MAX_THREADS-1)
			return THREAD_NOMORE;
		i++;
	}
	TCB[i].isValid = 1;
	TCB[i].index = i;
	TCB[i].next = NULL;
	/*create thread context*/
	getcontext(&TCB[i].thread_context);	
	/*change the context to point to fn(parg)*/
	TCB[i].thread_context.uc_mcontext.gregs[REG_RIP] = (greg_t)thread_stub;
	TCB[i].thread_context.uc_mcontext.gregs[REG_RDI] = (greg_t)fn;
	TCB[i].thread_context.uc_mcontext.gregs[REG_RSI] = (greg_t)parg;
	TCB[i].stack_memory = (void *)malloc(THREAD_MIN_STACK); /*set the stack pointer with alignment*/
	greg_t thread_stack_pointer = (greg_t)TCB[i].stack_memory;
	//TCB[i].stack_memory = thread_stack_pointer;
	if(thread_stack_pointer == (greg_t)NULL)
		return THREAD_NOMEMORY;
	thread_stack_pointer += THREAD_MIN_STACK;
	greg_t align = thread_stack_pointer%16;
	thread_stack_pointer = thread_stack_pointer - align;
	thread_stack_pointer -=8;/*because push %rbp pushes stack ptr up by 8 bits*/
	TCB[i].thread_context.uc_mcontext.gregs[REG_RSP]  = thread_stack_pointer;
	/*put in end of ready state*/
	if(readyQ.tail == NULL){
		readyQ.tail = &TCB[i];

	}else{
		readyQ.tail->next = &TCB[i];
		readyQ.tail = readyQ.tail->next;
	}
	if(readyQ.head == NULL){
		readyQ.head = readyQ.tail;
	}

	return i;
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 *  * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	
 	assert(ret == THREAD_NONE);
	exit(0);
}

Tid
thread_yield(Tid want_tid)
{

	int setcontext_flag = 0;
	
	if(stack_mem_to_be_deleted != NULL){
		free(stack_mem_to_be_deleted);
		stack_mem_to_be_deleted = NULL;
	}
	if(want_tid == THREAD_SELF)
		return current_thread_id;
	if(want_tid == current_thread_id)
		return current_thread_id;

	if(want_tid == THREAD_ANY){
		
		if(readyQ.head == NULL){
			return THREAD_NONE;
		}
		/*save the current thread context*/
		TCB[current_thread_id].next = NULL;
		struct thread *temp_head = readyQ.head;
		Tid copy_current_tid = current_thread_id;
		
		
		if(getcontext(&TCB[current_thread_id].thread_context)){
			return THREAD_FAILED;
		}
		
		if(setcontext_flag == 0){
			
						
			/*add to readyQ at the end*/
			readyQ.tail->next = &TCB[copy_current_tid];
			readyQ.tail = readyQ.tail->next;
			readyQ.tail->next = NULL;
			/*switch to the new thread wiz. head of readyQ*/
			/*pop*/
			readyQ.head = readyQ.head->next;
			
			current_thread_id = temp_head->index;
			setcontext_flag = 1;
			setcontext(&temp_head->thread_context);
		}
		
		return temp_head->index;
	}

	if(!thread_ret_ok(want_tid) || (want_tid > THREAD_MAX_THREADS) ||!(TCB[want_tid].isValid)){
		return THREAD_INVALID;
	}else{
		/*find the thread in readyQ*/
		struct thread *temp = readyQ.head;
		struct thread *pre = NULL;
		while(temp != NULL){
			if(temp->index == want_tid){
				
				
				Tid copy_current_tid = current_thread_id;
				
				/*save the thread and restore the new thread*/
				if(getcontext(&TCB[current_thread_id].thread_context)){
					return THREAD_FAILED;
				}
				
				if(setcontext_flag == 0){	
				
					/*add to readyQ at the end*/
					readyQ.tail->next = &TCB[copy_current_tid];
					readyQ.tail = readyQ.tail->next;
					readyQ.tail->next = NULL;
					if(pre == NULL){
						/*remove head*/
						readyQ.head = readyQ.head->next;
					}else{
						pre->next = temp->next;
					}
					
					setcontext_flag = 1;
					current_thread_id = want_tid;
					setcontext(&temp->thread_context);
				}
				return want_tid;
			}
			pre = temp;
			temp = temp->next;
		}

		if(readyQ.head == NULL){
			return THREAD_NONE;
		}
		return THREAD_INVALID;		
	}
	return THREAD_FAILED;
}


Tid
thread_exit()
{
	if(stack_mem_to_be_deleted != NULL){
		free(stack_mem_to_be_deleted);
		stack_mem_to_be_deleted = NULL;
	}
	if(readyQ.head == NULL)
		return THREAD_NONE;

	//Tid copy_current_thread_id = current_thread_id;
	TCB[current_thread_id].isValid = 0;	
	struct thread *temp_head = readyQ.head;
        //current_thread_id = temp_head->index;...this was the error that wasted your 10 hours _/\_
	readyQ.head = readyQ.head->next;	
	stack_mem_to_be_deleted = TCB[current_thread_id].stack_memory;
	current_thread_id = temp_head->index;
	setcontext(&temp_head->thread_context);
	//the program will not execute the assert(0) in this function	
	assert(0);
	
}

Tid
thread_kill(Tid tid)
{
	if(tid == current_thread_id || !(tid >= 0 && tid < THREAD_MAX_THREADS) || TCB[tid].isValid == 0)
		return THREAD_INVALID;
	if(readyQ.head == NULL)
		return THREAD_INVALID;

	struct thread *temp = readyQ.head;
	struct thread *pre = NULL;
	while(temp != NULL){
		if(temp->index == tid){
			/*remove the to be killed thread from the readyQ*/
			if(pre == NULL){
				readyQ.head = readyQ.head->next;
			}else{
				pre->next = temp->next;
			}
			if(temp->next == NULL)
				readyQ.tail = pre;
			
			TCB[tid].isValid = 0;			
			//
			if(tid !=0){
			free(TCB[tid].stack_memory);
			TCB[tid].stack_memory = NULL;
			}	

			return tid;
			
		}
		pre = temp;
		temp = temp->next;
	}

	return THREAD_INVALID;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
};

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
