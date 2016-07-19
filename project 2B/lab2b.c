#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "SortedList.h"

long long numiterations = 1;
long long numthreads = 1;
SortedList_t* head = NULL;
SortedListElement_t* elements = NULL;
char** keys;
int which_lock = 0;
pthread_mutex_t my_mutex;
int spinlock;
void thread_routine(void * arg);

int main(int argc, char* argv[])
{
	long long i;
	int j;
	//take parameters for options threads, iterations, yields, syncs
	char *yield;
	opterr = 0;
	int options;
	while(1)
	{
		static struct option long_options[] =
		{
			{"threads", required_argument, 0, 't'},
			{"iterations", required_argument, 0, 'i'},
			{"yield", required_argument, 0, 'y'},
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
			case 't':
				numthreads = atoi(optarg);
				break;
			case 'i':
				numiterations = atoi(optarg);
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
					pthread_mutex_init(&my_mutex, NULL);
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
				perror("Invalid option fed. Valid options are [--threads=<numthreads>], [--iterations=<numiterations>], [--yield=<X>");
				perror("where <numthreads> and <numiterations> are integers and X is a string containing 'ids'.");
				exit(1);
			default:
				perror("You should never get here");
				exit(1);
		}
	}
	
	//initialize an empty list
	head = malloc(sizeof(SortedList_t));
	head->next = head;
	head->prev = head;
	head->key = NULL;
	
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
	
	if(SortedList_length(head) != 0)
		fprintf(stderr, "ERROR: list length = %i\n", SortedList_length(head));
	
	long long numops = numthreads*numiterations*2;
	printf("%i threads x %i iterations x (insert + lookup/delete) = %i operations\n", numthreads, numiterations, numops);
	
	//calculate the elapsed time
	long long my_elapsed_time_in_ns = (my_end_time.tv_sec - my_start_time.tv_sec) * 1000000000;
	my_elapsed_time_in_ns += my_end_time.tv_nsec;
	my_elapsed_time_in_ns -= my_start_time.tv_nsec;
	
	//report data
	printf("elapsed time: %i nanoseconds\n", my_elapsed_time_in_ns);
	printf("per operation: %g nanoseconds\n\n", (double) ( (long double) my_elapsed_time_in_ns / numops ) );
	
	if(SortedList_length(head) != 0)
		exit(0);
	else
		exit(1);
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
			pthread_mutex_lock(&my_mutex);
			SortedList_insert(&head[0], &elements[index]);
			pthread_mutex_unlock(&my_mutex);
		}
	}
	else if(which_lock == 's')
	{
		for (k = 0; k != numiterations; k++)
		{
			long long index = (threadID * numiterations) + k;
			while(__sync_lock_test_and_set(&spinlock, 1));
			SortedList_insert(&head[0], &elements[index]);
			__sync_lock_release(&spinlock);
		}
	}
	else if(which_lock == 0)
	{
		//insert set of pre-allocated elements into shared list
		for (k = 0; k != numiterations; k++)
		{
			long long index = (threadID * numiterations) + k;
			SortedList_insert(&head[0], &elements[index]);
		}
	}
	else
	{
		perror("shouldn't get here"); exit(1);
	}
	
	
	int listlen;
	//get length
	if(which_lock == 'm')
	{
		pthread_mutex_lock(&my_mutex);
		listlen = SortedList_length(head);
		pthread_mutex_unlock(&my_mutex);
	}
	else if(which_lock == 's')
	{
		while(__sync_lock_test_and_set(&spinlock, 1));
		listlen = SortedList_length(head);
		__sync_lock_release(&spinlock);
	}
	else if(which_lock == 0)
	{
		//get list length
		listlen = SortedList_length(head);
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
			pthread_mutex_lock(&my_mutex);
			//looks up
			SortedListElement_t* node = SortedList_lookup(head, keys[index]);
			//deletes
			int ret = SortedList_delete(node);
			pthread_mutex_unlock(&my_mutex);
		}
	}
	else if(which_lock == 's')
	{
		for(k = 0; k != numiterations; k++)
		{
			SortedListElement_t* node;
			long long index = (threadID*numiterations) + k;
			while(__sync_lock_test_and_set(&spinlock, 1));
			//looks up
			 node = SortedList_lookup(head, keys[index]);
			//deletes
			int ret = SortedList_delete(node);
	
			__sync_lock_release(&spinlock);
		}
	}
	else if(which_lock == 0)
	{
			//looks up and deletes each of the prior keys entered
		for(k = 0; k != numiterations; k++)
		{
			long long index = (threadID*numiterations) + k;
			//looks up
			SortedListElement_t* node = SortedList_lookup(head, keys[index]);
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