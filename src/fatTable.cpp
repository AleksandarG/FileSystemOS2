#include "fatTable.h"
#include "part.h"
#include <iostream>

using namespace std;


FatTable::FatTable(ClusterNo size, char *buf)
{
	s = size;
	table = new long[size];

	memcpy(table, buf, size * 4);

}
void FatTable::formatTable(int size, int restrictedClusters)
{
	delete[] table;
	table = new long[size];

	for (ClusterNo i = 0; i < restrictedClusters; i++)
		table[i] = -1;
	for (ClusterNo i = restrictedClusters; i < size-2; i++)
	{
		table[i] = i + 1;
	}
	table[size - 1] = 0;

}

void FatTable::ispisiTabelu()
{
	for (ClusterNo i = 0; i < s; i++)
	{
		cout<<table[i]<<" " ;
	}
	cout << endl;
}

FatTable::~FatTable()
{
	delete[] table;
}
