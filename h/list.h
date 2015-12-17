#pragma once

#include <string>
#include "semaph.h"
using namespace std;

struct Elem {					

	string path;					
	int numOfThreads;				
	Elem* next;					
	FileMonitor* sem;

	Elem(string fn, int nT = 1, Elem* n = 0)
	{
		path = fn;
		
		numOfThreads = nT;
		next = n;
		sem = new FileMonitor();
	}
	~Elem()
	{
		delete sem;
	}
};


class List {
public:
	static bool	isOpen(string &fn);
	static void add(string &fn);
	static void remove(string &fn);
private:
	static Elem* first;
	static HANDLE mutexList;
};


