#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "SortedList.h"

//default parameters for input args for iteations, threads, lists
long long numiterations = 1;
long long numthreads = 1;
long long numlists = 1;

//the head will be an array of sublist heads to be inserted into
SortedList_t* head = NULL;
//elements will be an array of elements; each thread works with a consecutive number of elements
SortedListElement_t* elements = NULL;
//keys will be the array of char pts that point corresponding to the keys in elements
char** keys;

//which_lock determines which lock is used, while the other two are arrays of locks for ea subliss
int which_lock = 0;
pthread_mutex_t *my_mutex;
int *spinlock;

void thread_routine(void * arg);
int getSubListIndex(SortedListElement_t *element);



int main(int argc, char* argv[])
{
	//iterators
	long long i;
	int j;
	//used to store the input yield string to be parsed and eventually modify the extern yield
	char *yield;
	//take parameters for options threads, iterations, yields, syncs
	opterr = 0;
	int options;
	while(1)
	{
		static struct option long_options[] =
		{
			{"threads", required_argument, 0, 't'},
			{"iterations", required_argument, 0, 'i'},
			{"yield", required_argument, 0, 'y'},
			{"sync", required_argument, 0, 's'},
			{"lists", required_argument, 0, 'l'}
		};
		
		int option_index = 0;
		options = getopt_long(argc, argv, "", long_options, &option_index);
		
		if(options == -1)
			break;
			
		switch(options)
		{
			case 0:
				break;
			case 't':
				numthreads = atoi(optarg);
				break;
			case 'i':
				numiterations = atoi(optarg);
				break;
			case 'l':
				numlists = atoi(optarg);
				break;
			case 'y':
				if(optarg)
					yield = optarg;
				else
				{
					perror("Invalid argument sent to --yield. Valid options are [--yield=ids], where any inclusion of i, d, or s is allowed");
					exit(1);
				}
				i = 0;
				int run = 1;
				while(run)
				{
					switch(yield[i])
					{
						case 'i':
							//printf("i\n");
							opt_yield |= INSERT_YIELD;
							break;
						case 'd':
							//printf("d\n");
							opt_yield |= DELETE_YIELD;
							break;
						case 's':
							//printf("s\n");
							opt_yield |= SEARCH_YIELD;
							break;
						default:
							//printf("what\n");
							run = 0;
							break;
					}
					i++;
				}
				break;
			case 's':
				if(strcmp(optarg, "m") == 0)
				{
					which_lock = 'm';
					//pthread_mutex_init(&my_mutex, NULL); //in 2C need to do for multiple lists
				}
				else if(strcmp(optarg, "s") == 0)
					which_lock = 's';
				else
				{
					perror("Invalid argument sent to --sync. Valid options are [--sync=m] and [--sync=c");
					exit(1);
				}
				break;
			case '?':
				perror("Invalid option fed. Valid options are [--threads=<numthreads>], [--iterations=<numiterations>], [--lists=<numlists>], [--yield=<X>]");
				perror("where <numthreads>, <numiterations>, <numlists> are integers and X is a string containing 'ids'.");
				exit(1);
			default:
				perror("You should never get here");
				exit(1);
		}
	}
	
	
	
	//initialize numlists empty lists heads
	head = malloc(sizeof(SortedList_t) * numlists);
	for(i = 0; i != numlists; i++)
	{
		head[i].next = &head[i];
		head[i].prev = &head[i];
		head[i].key = NULL;
	}
	

	
	//initialize arrays of locks
	if(which_lock == 'm')
	{
		my_mutex = malloc(sizeof(pthread_mutex_t) * numlists);
		for(i = 0; i != numlists; i++)
			pthread_mutex_init(&my_mutex[i], NULL);
	}
	else if(which_lock == 's')
	{
		spinlock = malloc(sizeof(int) * numlists);
	}
	else if(which_lock == 0)
	{
		//do absolutely nothing;
	}
	else
	{
		perror("which_lock should be ONLY 'm', 's', or 0. otherwise, something is wrong");
		exit(1);
	}
	
	//initialize essentially a static array of characters which the key's values will point to
	char ASCII33To126[93];
	for (i = 0; i != 93; i++)
		ASCII33To126[i] = i + 33;
	
	
	//create and initialize w/ random keys required number of list elements
	//this is equal to (number of threads) * (number of iterations)
	long long numListElements = numthreads * numiterations;
	//allocate space for each list element structure
	elements = malloc(numListElements * sizeof(SortedListElement_t));
	//allocate space for each pointer to the key for each list element structue
	keys = malloc(numListElements * sizeof(char*));
	for (i = 0; i != numListElements; i++)
	{
		//generate each key. we ARBITRARILY use a key length 10 for each
		keys[i] = malloc(sizeof(char*) * 10);
		for (j = 0; j != 10; j++)
		{
			int character = rand() % 93;
			keys[i][j] = ASCII33To126[character];
		}
		elements[i].key = keys[i];
		elements[i].next = NULL;
		elements[i].prev = NULL;
	}
	
	
	//store threads here
	pthread_t pthreadarray[numthreads];
	
	//collect start time
	struct timespec my_start_time;
	clock_gettime(CLOCK_MONOTONIC, &my_start_time);
	
	//starts threads
	for(i = 0; i != numthreads; i++)
	{
		//pass i, thread number; each will use this to get which element segment its working w/
		int ret = pthread_create(&(pthreadarray[i]), NULL, (void *) &thread_routine, (void *)i);
		if(ret != 0)
		{
			fprintf(stderr, "failed to create thread %i\n", i);
			exit(1);
		}
	}
	
	//waits for threads to quit
	for(i = 0; i != numthreads; i++)
	{
		int ret = pthread_join(pthreadarray[i], NULL);
	}
	
	//collect end time
	struct timespec my_end_time;
	clock_gettime(CLOCK_MONOTONIC, &my_end_time);
	
	//get the final length of the WHOLE LIST (sum of all sublists) and output error if size != 0
	int final_listlen = 0;
	for(i = 0; i != numlists; i++)
		final_listlen += SortedList_length(&head[i]);
	if(final_listlen != 0)
		fprintf(stderr, "ERROR: list length = %i\n", final_listlen);
	
	long long numops = numthreads*numiterations*2;
	printf("%i threads x %i iterations x (insert + lookup/delete) = %i operations\n", numthreads, numiterations, numops);
	
	//calculate the elapsed time
	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec) * 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	
	//report data
	printf("elapsed time: %i nanoseconds\n", my_elapsed_time_in_ns);
	printf("per operation: %g nanoseconds\n\n", (double) ( (long double) my_elapsed_time_in_ns / numops ) );
	
	//exit with proper exit code depending on if there was an error in the whole list thingy
	if(final_listlen != 0)
		exit(0);
	else
		exit(1);
}

//gets the index of the sublist which to add the node to depending on the node's key
int getSubListIndex(SortedListElement_t *element)
{
	const char *key = element->key;
	int n = 0;
	int iterator;
	for(iterator = 0; iterator != 10; iterator++)
		n += (int) key[iterator];
	
	return n % numlists;
}

void thread_routine(void * arg)
{	
	long long threadID = (long long) arg;
	//printf("%i\n", threadID);
	int k;
	
	
	
	//insert
	if(which_lock == 'm')
	{
		for (k = 0; k != numiterations; k++)
		{
			long long index = (threadID * numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			pthread_mutex_lock(&my_mutex[sublistindex]);
			SortedList_insert(&head[sublistindex], &elements[index]);
			pthread_mutex_unlock(&my_mutex[sublistindex]);
		}
	}
	else if(which_lock == 's')
	{
		for (k = 0; k != numiterations; k++)
		{
			long long index = (threadID * numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			while(__sync_lock_test_and_set(&spinlock[sublistindex], 1));
			SortedList_insert(&head[sublistindex], &elements[index]);
			__sync_lock_release(&spinlock[sublistindex]);
		}
	}
	else if(which_lock == 0)
	{
		//insert set of pre-allocated elements into shared list
		for (k = 0; k != numiterations; k++)
		{
			long long index = (threadID * numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			SortedList_insert(&head[sublistindex], &elements[index]);
		}
	}
	else
	{
		perror("shouldn't get here"); exit(1);
	}
	
	
	int listlen = 0;
	//get the sum of the lengths of all sublists = the length of the ENTiRE list at THIS moment
	if(which_lock == 'm')
	{
		for(k = 0; k != numlists; k++)
		{
			pthread_mutex_lock(&my_mutex[k]);
			listlen += SortedList_length(&head[k]);
			pthread_mutex_unlock(&my_mutex[k]);
		}
	}
	else if(which_lock == 's')
	{
		for(k = 0; k != numlists; k++)
		{
			while(__sync_lock_test_and_set(&spinlock[k], 1));
			listlen += SortedList_length(&head[k]);
			__sync_lock_release(&spinlock[k]);
		}
	}
	else if(which_lock == 0)
	{
		//get list length
		for(k = 0; k != numlists; k++)
			listlen += SortedList_length(&head[k]);
	}
	else
	{
		perror("shouldn't get here"); exit(1);
	}
	
	//lookup + delete
	if(which_lock == 'm')
	{
		for(k = 0; k != numiterations; k++)
		{
			long long index = (threadID*numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			pthread_mutex_lock(&my_mutex[sublistindex]);
			//looks up
			SortedListElement_t* node = SortedList_lookup(&head[sublistindex], keys[index]);
			//deletes
			int ret = SortedList_delete(node);
			pthread_mutex_unlock(&my_mutex[sublistindex]);
		}
	}
	else if(which_lock == 's')
	{
		for(k = 0; k != numiterations; k++)
		{
			SortedListElement_t* node;
			long long index = (threadID*numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			while(__sync_lock_test_and_set(&spinlock[sublistindex], 1));
			//looks up
			node = SortedList_lookup(&head[sublistindex], keys[index]);
			//deletes
			int ret = SortedList_delete(node);
	
			__sync_lock_release(&spinlock[sublistindex]);
		}
	}
	else if(which_lock == 0)
	{
			//looks up and deletes each of the prior keys entered
		for(k = 0; k != numiterations; k++)
		{
			long long index = (threadID*numiterations) + k;
			int sublistindex = getSubListIndex(&elements[index]);
			//looks up
			SortedListElement_t* node = SortedList_lookup(&head[sublistindex], keys[index]);
			//deletes
			int ret = SortedList_delete(node);
		}
	}
	else
	{
		perror("shouldn't get here"); exit(1);
	}
	
	pthread_exit(NULL);
}