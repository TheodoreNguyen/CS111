#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>

//declare some global vars for ur implementation SUCH AS counter, lock, mutex, et
long long counter = 0;
int numiterations = 1;
int numthreads = 1;
int opt_yield = 0;
int which_lock;
int sync = 0;
pthread_mutex_t my_mutex; 	//PTHREAD_MUTEX_INITIALIZER;
int spinlock;

// ADD VERSION FROM SPEC - VERSION 2, with YIELD()
void add(long long *pointer, long long value);
// thread function to run a test - performs addition with adding. initial implementation before synchronization. used for debugging
void thread_routineinitial(int* threadnum);					
//main thread function
void thread_routine();


int main(int argc, char* argv[])
{
	//initialize variables in case needed
	int i;									
	long long numops;
	
	
	//use getopt_long() to process options
	opterr = 0;
	int options;
	while(1)
	{
		static struct option long_options[] =
		{
			{"threads", required_argument, 0, 't'},
			{"iterations", required_argument, 0, 'i'},
			{"yield", no_argument, &opt_yield, 1},
			{"sync", required_argument, 0, 's'}
		};
		
		int option_index = 0;
		options = getopt_long(argc, argv, "", long_options, &option_index);
		
		if(options == -1)
			break;	
		
		switch(options)
		{
			case 0:
				break;
			case 's':
				sync = 1;
				if(strcmp(optarg, "m") == 0)
				{
					which_lock = 'm';
					pthread_mutex_init(&my_mutex, NULL);
				}
				else if (strcmp(optarg, "s") == 0)
					which_lock = 's';
				else if (strcmp(optarg, "c") == 0)
					which_lock = 'c';
				else
				{
					perror("Invalid argument sent to --sync. Valid options are [--sync=m], [--sync=s], and [--sync=c]");
					exit(1);
				}
				break;
			case 't':
				numthreads = atoi(optarg);
				break;
			case 'i':
				numiterations = atoi(optarg);
				break;
			case '?':
				perror("Invalid option fed. Valid options are [--threads=<numthreads>], [--iterations=<numiterations>], [--yield], or [--sync=<X>]");
				perror("<numthreads> and <numiterations> are integers and X is a character 'm', 's', or 'c'");
				perror("Running the program without --threads or --iterations will set their default value to 1 when run");
				exit(1);
			default:
				perror("You should never get here");
				exit(1);
		}
	}
	//determine total number of ops
	numops = numthreads * numiterations * 2;
	
	//create place to store threads
	pthread_t pthreadarray[numthreads];
	
	
	//this is used for debugging
	//int pthreadnums[numthreads];
	//for(i = 0; i != numthreads; i++)
	//	pthreadnums[i] = i;
	
	
	//collect start time
	struct timespec my_start_time;
	clock_gettime(CLOCK_MONOTONIC, &my_start_time);
	
	
	//start threads
	for(i = 0; i != numthreads; i++)
	{
		//printf("we are now creating thread number %i\n", i);
		int ret = pthread_create(&(pthreadarray[i]), NULL, (void *) &thread_routine, NULL);
		//check ret if needed
		if(ret != 0)
		{
			fprintf(stderr, "Failed to create thread %i\n", i);
			exit(1);
		}
	}
	
	//wait for threads to quit
	for (i = 0; i != numthreads; i++)
	{
		//printf("Waiting for thread %i to exit\n", i);
		int ret = pthread_join(pthreadarray[i], NULL);
		//printf("the %ith thread has successfully joined with parent\n", i);
		//check ret value to see if thread exited successfully
	}
	
	
	//collect end time
	struct timespec my_end_time;
	clock_gettime(CLOCK_MONOTONIC, &my_end_time);
	
	
	printf("%i threads x %i iterations x (add + subtract) = %i operations.\n", numthreads, numiterations, numops);
	
	
	if(counter != 0)
		fprintf(stderr, "ERROR: final count = %i\n", counter);
	
	
	//calculate the elapsed time
	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec) * 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	
	
	//report data
	printf("elapsed time: %i nanoseconds\n", my_elapsed_time_in_ns);
	printf("per operation: %g nanoseconds\n\n", (double) ( (long double) my_elapsed_time_in_ns / numops ) );
	
	if(counter != 0)
		exit(1);
	else
		exit(0);
}



void thread_routine()
{
	int i;
	if(sync)
	{	//perform +1 numiterations times
		for(i = 0; i != numiterations; i++)
		{
			switch(which_lock)
			{
				case 'm':		//mutex
				{
					pthread_mutex_lock(&my_mutex);
					add(&counter, 1);
					pthread_mutex_unlock(&my_mutex);
					break;
				} 
				case 's':		//spin
				{
					while(__sync_lock_test_and_set(&spinlock, 1));
						add(&counter, 1);
					__sync_lock_release(&spinlock);
					break;
				} 
				case 'c':		//compare and swap
				{	
					long long prev;
					long long sum;
					do
					{
						prev = counter;
						sum = prev;
						add(&sum, 1);
					} while(__sync_val_compare_and_swap(&counter, prev, sum) != prev);
					break;
				}
			}
		}
		//perform -1 numiterations times
		for(i = 0; i != numiterations; i++)
		{
			switch(which_lock)
			{
				case 'm':		//mutex
				{
					pthread_mutex_lock(&my_mutex);
					add(&counter, -1);
					pthread_mutex_unlock(&my_mutex);
					break;
				} 
				case 's':		//spin
				{
					while(__sync_lock_test_and_set(&spinlock, 1));
						add(&counter, -1);
					__sync_lock_release(&spinlock);
					break;
				} 
				case 'c':		//compare and swap
				{
					long long prev;
					long long sum;
					do
					{
						prev = counter;
						sum = prev;
						add(&sum, -1);
					} while(__sync_val_compare_and_swap(&counter, prev, sum) != prev);
					break;
				}
			}
		}
	}
	else
	{
		for(i = 0; i != numiterations; i++)
			add(&counter, 1);
		for(i = 0; i != numiterations; i++)
			add(&counter, -1);
	}
	pthread_exit(NULL);
}


// ADD VERSION FROM SPEC - VERSION 2, with YIELD()
void add(long long *pointer, long long value){
	long long sum = *pointer + value;
	if(opt_yield)
		pthread_yield();
	*pointer = sum;
}



// thread function to run a test - performs addition with adding. initial implementation before synchronization
void thread_routineinitial(int* threadnum)					
{
	//printf("We are now executing newly created thread number %i\n", *threadnum);
	int i;
	for(i = 0; i != numiterations; i++)
		add(&counter, 1);
	for(i = 0; i != numiterations; i++)
		add(&counter, -1);
	//printf("thread number %i is exiting now\n", *threadnum);
	pthread_exit(NULL);
}



