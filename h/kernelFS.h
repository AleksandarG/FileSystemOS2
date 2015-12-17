// File: kernelFS.h
#pragma once

#include "fs.h"
#include "part.h"

#include <Windows.h>

#define MutexSignal(x) ReleaseSemaphore(x,1,NULL)
#define MutexWait(x) WaitForSingleObject(x,INFINITE)

using namespace std;


class Drive;

class KernelFS
{
public:
	KernelFS();

	~KernelFS();

	char mount(Partition* partition);

	char unmount(char part);

	char format(char part);
	
	char doesExist(char* fname);

	File* open(char* fname, char mode);

	char deleteFile(char* fname);
	
	char createDir(char* dirname);

	void izlistaj(char *dirName); //dodatno
	char deleteDir(char* dirname); 

	char readDir(char* dirname, EntryNum n, Entry &e);
	static Drive* drives[26];
private:
	static HANDLE KernelFSmutex;
	static HANDLE KernelFSMountFormat;
	
};

