#include "List.h"


Elem* List::first = 0;
HANDLE List::mutexList = CreateSemaphore(NULL, 1, 1, NULL);


bool List::isOpen(string &fn){

	WaitForSingleObject(mutexList, INFINITE);
	
	for (Elem* current = first; current != NULL; current = current->next)
	{
		if (!current->path.compare(fn))
		{
			ReleaseSemaphore(mutexList, 1, NULL);
			return 1;
		}
	}
	
	ReleaseSemaphore(mutexList, 1, NULL);
	return 0;
}

void List::add(string &fn){
	WaitForSingleObject(mutexList, INFINITE);
	
	for (Elem* current = first; current != NULL; current = current->next)
	{
		if (!current->path.compare(fn))
		{
			current->numOfThreads++;
			ReleaseSemaphore(mutexList, 1, NULL);
			
			 current->sem->startUsing(); 
			return;
		}
	}
	

	Elem* newElem = new Elem(fn, 1);

	if (!first)									
	{
		first = newElem;
	}
	else {
		newElem->next = first;					
		first = newElem;
	}
	
	
	
	newElem->sem->startUsing(); 
	ReleaseSemaphore(mutexList, 1, NULL);
}

void List::remove(string &fn){
	WaitForSingleObject(mutexList, INFINITE);
	
	for (Elem* current = first; current != NULL; current = current->next)
	{
		if (!current->path.compare(fn))
		{

			current->numOfThreads--;
			ReleaseSemaphore(mutexList, 1, NULL);
			
			current->sem->stopUsing();
			
			if (current->numOfThreads)									
			{
				return;
			}
			else
			{
				WaitForSingleObject(mutexList, INFINITE);
				
				Elem *curr = first, *last = 0;

				while (curr)
				if (!curr->path.compare(fn))
				{
					Elem* old = curr;
					curr = curr->next;
					if (!last) first = curr;
					else last->next = curr;
					delete old;

					ReleaseSemaphore(mutexList, 1, NULL);
					return;
				}
				else
				{
					last = curr; curr = curr->next;						
				}
			}
		}
	}
}