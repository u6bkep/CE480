
#include <stdio.h>    // needed to print to the screen
#include <stdlib.h>   // ascii to integer conversion


//#include <Windows.h>  // for sleep function
//#include <process.h>  // Windows multithreading functions

#include <unistd.h>
#include <pthread.h>


#include <time.h>

#define NTHREADS 10

// when creating a new thread, only one "thing" can be passed to it. However, if you make a struct with multiple fields,
// you can send one thing that has as many different fields as you need. Below, thread_t is being defined so that two values
// can be passed.
typedef struct{
	int tid;	// numerical ID of the thread
	int r;		// randomly generated integer
	int userNumber;
} thread_t;

thread_t tids[];
pthread_t ptids[];

int threadCount = 0;
int reprintCount = 0;

// function prototypes
void printThread(void *myID);

int main(int argc, char *argv[])
{	
	int i;
	if (argc == 2)  // assumes correct input
	{
		//printf("You entered the number %d on the command line from program %s.\n", atoi(argv[1]), argv[0]);
		printf("no threads specified, exiting.\n");
		exit(EXIT_FAILURE);

	}

	reprintCount = atoi(argv[1]);
	threadCount = argc - 2;

	thread_t* tids =  malloc(threadCount*sizeof(thread_t));
	pthread_t* ptids =  malloc(threadCount*sizeof(pthread_t));

	srand((int)time(NULL)); // cheap way of generating a different random seed each time

	printf("This is the main program. spawning %d threads with %d reprints.\n", threadCount, reprintCount);
	for (i = 0; i < threadCount; i++)
	{
		tids[i].tid = i;
		tids[i].r = rand();
		tids[i].userNumber = atoi(argv[i+2]);
		printf("launching thread %d...\n", tids[i].tid);
		//_beginthread(printThread, 0, &tids[i]);   // name of function, stack size (0 for default), pointer to passed parameter
		pthread_create(&ptids[i], NULL, printThread,(void *) &tids[i]);
		sleep(.5);
	}
	for (i = 0; i < threadCount; i++)
	{
		pthread_join(ptids[i], NULL);  // wait long enough for all threads to finish
	}

	//sleep(20);  
}

void printThread(void *pMyID) // pointer to void let's you pass one pointer to absolutely anything...however, since the type is "void",
							  // the compiler has no way to treat it as anything more useful, like a struct or array, so we will assign the 
							  // pointer to void to a pointer to an object of the correct type, and now the compiler can access the 
							  // fields. This is a little cleaner than type-casting the void pointer EVERY time it is used.
{               
	thread_t* MyID = pMyID;   // somewhat clean way to convert the void input to a typed input
	int i = reprintCount;
	while(i)
	{
		sleep( (MyID->r % 100) * .05);
		printf("I am thread %d, printing number: %d\n", MyID->tid, MyID->userNumber);
		i--;
	}
	
}