#include "SortedList.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int opt_yield = 0;

//given the head of a list "*list" and a list element struct "*element", want to
//add the "*element" to the correct position in the list, obviously
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	//initialize pointer to the current list head as "previous"
	SortedListElement_t *previous = list;
	//initialize pointer to the node right after the current list head
	SortedListElement_t *next = list->next;
	
	//while the next node is not equal to the head node of the list, do the following
	while(next != list)
	{
		//compare the new node's key with the next node's key. If the next nodes key
		//is greater than or equal to the new nodes, we want to insert here and so
		//we break to go to the insertion stage
		if(strcmp(element->key, next->key) <= 0)
			break;
		//if the new node and next node's key do not satisfy the condition, move onto
		//the next node
		previous = next;
		next = next->next;
	}
	
	//opt_yield thing
	if(opt_yield & INSERT_YIELD)
		pthread_yield();
	
	//we insert the new node into the last put off position
	//set the new node's prev and next pointers to the current ones
	element->prev = previous;
	element->next = next;
	//set the surrounding nodes - the previous node's next pointer to the element
	previous->next = element;
	//and the next's node previous pointer to the element
	next->prev = element;
	
}

int SortedList_delete(SortedListElement_t *element)
{
	//first get copies for pointers of the two adjacent nodes
	SortedListElement_t *previous = element->prev;
	SortedListElement_t *Next = element->next;
	
	//then check if the adjacent nodes actually point to it
	if(Next->prev != element || previous->next != element)
		return 1;
	
	//opt_yield thing
	if(opt_yield & DELETE_YIELD)
		pthread_yield();
	
	//remove the element from the list
	previous->next = Next;
	Next->prev = previous;
	element->next = NULL;
	element->prev = NULL;
	
	return 0;	
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
	SortedListElement_t* iterator;
	SortedListElement_t* desiredNode = NULL;
	//if(opt_yield & SEARCH_YIELD)
	//	pthread_yield();
	
	for(iterator = list->next; iterator != list; iterator = iterator->next)
	{
		if(opt_yield & SEARCH_YIELD)
			pthread_yield();
		if(strcmp(iterator->key, key) == 0)
		{
			desiredNode = iterator;
			break;
		}
	}
	
	return desiredNode;
}

int SortedList_length(SortedList_t *list)
{
	int listlen = 0;
	SortedListElement_t *iterator;
	
	//if(opt_yield & SEARCH_YIELD)
	//	pthread_yield();
	
	for(iterator = list->next; iterator != list; iterator = iterator->next)
	{
		if(opt_yield & SEARCH_YIELD)
			pthread_yield();
		listlen++;
	}
	return listlen;
}

void printListKeys(SortedList_t *list)
{
	int elementNum = 0;
	SortedListElement_t *iterator;
	for(iterator = list->next; iterator != list; iterator = iterator->next)
	{
		printf("%s ", iterator->key);
		//printf("The key at the %ith element of the list at memory address %p is %s.\n", elementNum, iterator, iterator->key);
		elementNum++;
	}
	printf("\n");
}
