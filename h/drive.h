// File: drive.h
#pragma once
#include "part.h"
#include "fs.h"
#include "kernelFile.h"
#include <string>
#include <Windows.h>

using namespace std;

class Partition;
class File;
class FatTable;
class Ticket;
 
class Drive 
{
public:
	Drive(Partition *, char);
	~Drive();

	Partition* getPartition() const;
	char getLetter() const;

	void printD();

	void format();

	KernelFile* openFile(string &fName, char mode);



	std::pair<int, int> doesExist(string &fname, int mode, Entry *root);  //0 za obicno pretrazivanje, 1 ako se zove zbog kreiranja fajla/direktorijuma

	char createDirectory(string &dirName);

	void showEntries(string &dirName);

	char readDir(string &dirname, EntryNum n, Entry &e);
	char deleteDir(string &dirname);
	char deleteFile(string &fName);
	pair<int, int> Drive::doesExistFile(string &fName);
	void updateClusters();



	
private:
	void createFile(string& fName, Entry &e);

	
	void readEntry(char* buffer, EntryNum n, Entry &e);
	void makeDirectory(Entry &rootDir, Entry &dir);
	//int getRoots(string &dirName, Entry& root, Entry &parent, Entry &target);

	ClusterNo getFreeCluster();
	void setClusterFree(ClusterNo);


	Partition* partition;
	char letter;
	FatTable *fat;
	ClusterNo freeClusterRoot;  // pocetak liste slobodnih klastera
	ClusterNo fatSize;
	ClusterNo rootDirectory;
	ClusterNo rootDirectorySize;

	int restrictedClusters;
	int numOfOpenedFiles;
	int fileOpenPermission;
	HANDLE safeToFormat;
	friend class KernelFS;
	friend class KernelFile;
};

