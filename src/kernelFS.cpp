// File: kernelFS.cpp

#include <iostream>
#include "kernelFS.h"
#include "drive.h"
#include "file.h"
#include "fatTable.h"

using namespace std;

Drive *KernelFS::drives[26] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

HANDLE KernelFS::KernelFSmutex = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE KernelFS::KernelFSMountFormat = CreateSemaphore(NULL, 1, 1, NULL);

KernelFS::KernelFS()
{
	
}

KernelFS::~KernelFS()
{

	for (int i = 0; i < 26; i++)
		if (drives[i])delete drives[i];

}

char KernelFS::mount(Partition* partition)
{
	MutexWait(KernelFSmutex);
	int i = 0;
	char driveLetter = '0';
	while (i < 26 && drives[i] != 0)
	{
		if (drives[i]->getPartition() == partition)
		{
			MutexSignal(KernelFSmutex);
			return drives[i]->getLetter();
		}
			
		i++;
	}

	if (i >= 26)
	{
		MutexSignal(KernelFSmutex);
		return driveLetter;
	}
	driveLetter = 'A' + i;
	Drive* newDrive = new Drive(partition, driveLetter);
	drives[i] = newDrive;
	
	MutexSignal(KernelFSmutex);
	return driveLetter;
}


char KernelFS::format(char part)
{
	//MutexWait(KernelFSmutex);
	//MutexWait(KernelFSMountFormat);
	drives[part - 'A']->format();
	//MutexWait(KernelFSMountFormat);
	//MutexSignal(KernelFSmutex);
	return 1;
}

char KernelFS::unmount(char part)
{
	MutexWait(KernelFSmutex);
	if (part<'A' || part>'Z')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}

	//drives[part - 'A']->updateClusters();
	//drives[part - 'A']->fat->ispisiTabelu();
	MutexSignal(KernelFSmutex);
	delete drives[part - 'A'];
	MutexWait(KernelFSmutex);
	//dodati wait dok se ne zatvore svi fajlovi


	drives[part - 'A'] = 0;
	//if (d) delete d;
	MutexSignal(KernelFSmutex);
	return 1;
}

char KernelFS::doesExist(char* fname)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[fname[0] - 'A'];
	if (d == nullptr)
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
		

	if (fname[1] != ':' || fname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	char* di = fname + 3;
	string dd=  string(di);
	if (dd.find_last_of(".") != -1)
	{
		char c= (d->doesExistFile(dd).first == -1) ? 0 : 1;
		MutexSignal(KernelFSmutex);
		return c;
	}	
	else
	{
		char c = (d->doesExist(dd, 1, NULL).first == -1) ? 0 : 1;
		MutexSignal(KernelFSmutex);
		return c;
	}
	
}

void KernelFS::izlistaj(char *dirName)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[dirName[0] - 'A'];
	if (d == nullptr)
		cout<<"ne postoji\n";

	if (dirName[1] != ':' || dirName[2] != '\\')
		cout << "ne postoji\n";
	char* di = dirName + 3;
	string dd = string(di);
	d->showEntries(dd);
	MutexSignal(KernelFSmutex);
}

File* KernelFS::open(char* fname, char mode)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[fname[0] - 'A'];
	if (d == nullptr)
	{
		MutexSignal(KernelFSmutex);
		return NULL;
	}
			
	
	if (fname[1] != ':' || fname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return NULL;
	}
		
	char* di = fname + 3;
	string dd = string(di);
	MutexSignal(KernelFSmutex);
	KernelFile *k= d->openFile(dd, mode);
	MutexWait(KernelFSmutex);
	if (k != NULL)
	{
		File *f = new File();
		f->myImpl = k;
		MutexSignal(KernelFSmutex);
		return f;
	}
	MutexSignal(KernelFSmutex);
	return NULL;
}


char KernelFS::createDir(char* dirname)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[dirname[0] - 'A'];
	if (d == 0)
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
		

	if (dirname[1] != ':' || dirname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	char* di = dirname + 3;
	string dd = string(di);
	
	char c= d->createDirectory(dd);
	MutexSignal(KernelFSmutex);
	return c;
}

char KernelFS::deleteDir(char* dirname)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[dirname[0] - 'A'];
	if (d == 0)
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	if (dirname[1] != ':' || dirname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	char* di = dirname + 3;
	string dd = string(di);
	char c= d->deleteDir(dd);
	MutexSignal(KernelFSmutex);
	return c;
}

char KernelFS::readDir(char* dirname, EntryNum n, Entry &e)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[dirname[0] - 'A'];
	if (d == 0)
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	if (dirname[1] != ':' || dirname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	char* di = dirname + 3;
	string dd = string(di);
	char c= d->readDir(dd, n, e);
	MutexSignal(KernelFSmutex);
	return c;
}

char KernelFS::deleteFile(char* fname)
{
	MutexWait(KernelFSmutex);
	Drive* d = drives[fname[0] - 'A'];
	if (d == 0)
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}

	if (fname[1] != ':' || fname[2] != '\\')
	{
		MutexSignal(KernelFSmutex);
		return 0;
	}
	char* di = fname + 3;
	string dd = string(di);
	char c= d->deleteFile(dd);
	MutexSignal(KernelFSmutex);
	return c;
}