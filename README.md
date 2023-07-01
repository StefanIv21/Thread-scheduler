# Thread scheduler
 - implements a thread scheduler that controls their execution in user-space. It simulates a preemptive process scheduler, in a uniprocessor system, that uses a Round Robin scheduling algorithm with priorities.

- The implementation of the thread scheduler is done in a dynamic shared library, which is loaded by the threads to be scheduled. It exports the following functions:

      INIT - initializes internal scheduler structures.
      FORK - starts a new thread.
      EXEC - simulates the execution of an instruction.
      WAIT - waits for an I/O event/operation.
      SIGNAL - signals threads waiting for an I/O event/operation.
      END - destroys the scheduler and frees the allocated structures.


## Build:
The local directory must contain the thread scheduler library
(libscheduler.so). Use the Makefile.checker to properly build the run_test
executable:

	make -f Makefile.checker

This command will also run all tests and print out the results.

## Run:

In order to run the test suite you can either use the run_all.sh script or
the run_test executable. The loader must be able to locate the library
(libscheduler.so) (use LD_LIBRARY_PATH).

The run_all.sh script runs all tests and computes assignment grade (95 points
maximum):

	LD_LIBRARY_PATH=. ./run_all.sh

In order to run a specific test, pass the test number (1 .. 20) to the run_test
executable:

	LD_LIBRARY_PATH=. ./_test/run_test 1

## Implementation
  I used the following 2 structures:
- The "thread" structure: besides the usual fields (id, priority, handler and io) I added a traffic light
         to be able to plan the execution order of the thread and three pointers to this structure with the following purposes:
                
              the "urm_priority" field points to the threads in the "priority_queue" queue (the main queue), depending on their priority
              the "urm_threads" field points to all threads created in the order of the forks, the field being used
                     by the "threads_queue" list in the so_end function to wait for all threads to finish
              the "urm_free" field points to all threads created in the order of the forks, the field being used
                     by the "free_queue" list in the so_end function to release the memory occupied by threads

- The "scheduler" structure contains the number of supported events, the amount of time and a pointer to the thread structure that keeps track of which thread is running

To count the number of threads in the "priority_queue" priority queue, we used the "count" variable I used a vector of threads in which I put the threads blocked by the so_wait function

  In addition to the main functions, I have also implemented
  
         - the "add_queue" function that adds threads to the "priority_queue" queue according to their priority
         - the "extract" function extracts the first thread from the "priority_queue" queue
         - the "switchthread" function - extracts the first thread from the "priority_queue" queue
                                 - puts it into running, notifying it that it can run with the "sem_post" function
         - the "scheduler" function plans which thread will run by making the following checks:
              - in case there are threads in the priority queue:
                  if the current thread has not finished but there is another thread with a higher priority, I make the exchange:
                          I add the current thread to the queue, block it with the sem_wait function and start the first thread in the queue
                  if the current thread has finished, but has the highest priority, it continues, adding time
                  otherwise I add the current thread to the queue, block it with the sem_wait function and start the first thread in the queue

### so_init
- I check the received parameters, and if everything is ok, I allocate memory for the scheduler and initialize the global variables

### so_fork
- allocate memory to a new thread and initialize it with received parameters
- add it to the lists where priorities do not matter, only the presence of threads (the lists will be used in the so_end function)
                     (threads_queue and free_queue)
- initialize the semaphore for each thread (with its help I will determine the scheduling of the threads)
- add to the queue with priorities and create the thread (the function received as an argument is "start_thread")
- start_thread function:
   
        blocks the thread until it is scheduled, with the sem_wait function
        after it is notified by another thread and can run, it executes the received routine
        after it finishes its execution and there are still threads in the priority queue, the switchthread function is called
  
- if it is the first element in the queue with priorities, it is put directly into running with the switchthread function
           otherwise, I decrease the amount of time and call the scheduler (scheduler function).

### so_end
- go through the "threads_queue" list to wait for all threads to finish
- go through the "free_queue" list to free the memory occupied by each thread with the respective semaphore
- release the scheduler

### so_wait
- set the running thread to the respective io
- add it to the "blocked_queue" vector
- block it with the sem_wait function and call the "switchthread" function to run a new thread
- in case the array "blocked_queue" remains out of memory, I reallocate it

### so_signal
- go through the "blocked_queue" vector, and if the thread has the same io as the io received as an argument, I add it to the priority queue
- reschedule by calling the "scheduler" function


