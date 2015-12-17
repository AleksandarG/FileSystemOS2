#pragma once
#include "part.h"



class FatTable
{
public:
	FatTable(ClusterNo, char*);
	~FatTable();

	//void setTable();
	void formatTable(int, int);
	void ispisiTabelu();
	//void setEntryTo
private:
	friend class Drive;
	friend class KernelFile;
	long* table;
	int s;
};

