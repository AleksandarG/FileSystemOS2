#define _CRT_SECURE_NO_WARNINGS
#include "kernelFile.h"
#include "kernelFS.h"
#include "drive.h"
#include "fatTable.h"

#include "list.h"
#include <string>
#include <Windows.h>


using namespace std;

KernelFile::KernelFile(string name, BytesCnt cursor, BytesCnt size, char m, ClusterNo first,  char drive)
{
	fileName = name;
	fileSize = size;
	cursorPosition = cursor;
	mode = m;
	firstCluster = first;
	myDrive = drive - 'A';
	bufferedCluster = new char[ClusterSize];
	//fileMutex = CreateSemaphore(NULL, 1, 1, NULL);
	currentBuffered = 0;
	positionInCluster = 0;
	dirty = 0;
	if (fileSize > 0)
		seek(cursorPosition);
	KernelFS::drives[myDrive]->numOfOpenedFiles++;
	if (KernelFS::drives[myDrive]->numOfOpenedFiles==1)
		WaitForSingleObject(KernelFS::drives[myDrive]->safeToFormat, INFINITE); //onemoguci formatiranje i unmountovanje
	//seek
}


KernelFile::~KernelFile()
{
	if (dirty)
	{
		KernelFS::drives[myDrive]->partition->writeCluster(currentBuffered, bufferedCluster);
		dirty = 0;
	}
	pair<int, int> p = KernelFS::drives[myDrive]->doesExistFile(string(fileName)); //
	//char *buff = new char[ClusterSize];
	KernelFS::drives[myDrive]->partition->readCluster(p.first, bufferedCluster);

	memcpy(bufferedCluster + p.second * 20 + 12, &firstCluster, 4);
	memcpy(bufferedCluster + p.second * 20 + 16, &fileSize, 4);

	KernelFS::drives[myDrive]->partition->writeCluster(p.first, bufferedCluster);
	delete[] bufferedCluster;
	
	KernelFS::drives[myDrive]->numOfOpenedFiles--;
	if (KernelFS::drives[myDrive]->numOfOpenedFiles == 0)
		ReleaseSemaphore(KernelFS::drives[myDrive]->safeToFormat, 1, NULL); //nema otvorenih fajlova, pa moze da se dozvoli format i unmount
	List::remove(string(fileName));
}

char KernelFile::write(BytesCnt bytesCount, char* buffer)
{
	
	if (bytesCount <= 0 || mode == 'r')
	{
		
		return 0; //pogresni parametri
	}

	//int currentClusterNumber;
	int t;
	if (fileSize == 0)
	{
		t=KernelFS::drives[myDrive]->getFreeCluster();
		if (t == 0)return 0; //nema slobodnih klastera
		
		firstCluster = currentBuffered=t;
		//KernelFS::drives[myDrive]->partition->readCluster(currentClusterNumber, bufferedCluster);
		dirty = 0;
	}

	int bytesLeft = bytesCount;
	int spaceLeft = 2048 - positionInCluster;
	int pomeraj = 0;
	while (bytesLeft > 0)
	{
		if (spaceLeft > bytesLeft)
		{
			memcpy(bufferedCluster + positionInCluster, buffer + pomeraj, bytesLeft);
			dirty = 1;
			spaceLeft -= bytesLeft;
			positionInCluster += bytesLeft;
			bytesLeft = 0;

		}
		else
		{
			memcpy(bufferedCluster + positionInCluster, buffer + pomeraj, spaceLeft);
			//dirty = 1;
			bytesLeft -= spaceLeft;
			pomeraj += spaceLeft;


			KernelFS::drives[myDrive]->partition->writeCluster(currentBuffered, bufferedCluster);
			dirty = 0;

			//spaceLeft = 0;
			
			if (t = KernelFS::drives[myDrive]->fat->table[currentBuffered])
			{
				
				currentBuffered = t;
				
			}
			else if (t = KernelFS::drives[myDrive]->getFreeCluster())
			{
				
				KernelFS::drives[myDrive]->fat->table[currentBuffered] = t;
				KernelFS::drives[myDrive]->updateClusters();
				currentBuffered = t;
			}
				

			KernelFS::drives[myDrive]->partition->readCluster(currentBuffered, bufferedCluster);
			//cursorPosition += spaceLeft;
			positionInCluster = 0;
			spaceLeft = ClusterSize;

		}
	}

	if (cursorPosition + bytesCount > fileSize)
		fileSize = (cursorPosition + bytesCount);
	cursorPosition += bytesCount;
	



	return 1;
}

BytesCnt KernelFile::read(BytesCnt bytesCount, char* buffer)
{
	if (cursorPosition == fileSize)return 0; //kursor je na kraju fajla
	
	int spaceLeft = 2048 - positionInCluster;
	int readBytes = 0;
	
	int bytesLeft = bytesCount;

	

	if (cursorPosition + bytesLeft > fileSize)
		readBytes =bytesLeft= fileSize - cursorPosition;
	else readBytes = bytesLeft;
	int pomeraj=0;
	while (bytesLeft > 0)
	{
		if (spaceLeft > bytesLeft)
		{
			memcpy(buffer+ pomeraj, bufferedCluster + positionInCluster, bytesLeft);
			spaceLeft -= bytesLeft;
			positionInCluster += bytesLeft;
			bytesLeft = 0;

		}
		else
		{
			memcpy(buffer + pomeraj, bufferedCluster + positionInCluster, spaceLeft);
			bytesLeft -= spaceLeft;
			pomeraj += spaceLeft;
			//spaceLeft = 0;
			int t;
			if (t = KernelFS::drives[myDrive]->fat->table[currentBuffered])
			{
				if (dirty)
				{
					KernelFS::drives[myDrive]->partition->writeCluster(currentBuffered, bufferedCluster);
					dirty = 0;
				}
				currentBuffered = t;
				KernelFS::drives[myDrive]->partition->readCluster(currentBuffered, bufferedCluster);
				//cursorPosition += spaceLeft;
				positionInCluster = 0;
				spaceLeft = ClusterSize;
				
			}
			
		}
	}
	cursorPosition += readBytes;
	
	
	return readBytes;
}

char KernelFile::seek(BytesCnt position)
{
	if (position<0 || position>fileSize)
		return 0; //van opsega
	cursorPosition = position;
	positionInCluster = cursorPosition % 2048;
	int tmp = cursorPosition / 2048;
	
	int i = 0;
	int t = firstCluster;


	while (i < tmp)
	{
		t = KernelFS::drives[myDrive]->fat->table[t];
		i++;
	}
	if (t != currentBuffered)
	{
		if (dirty)
		{
			KernelFS::drives[myDrive]->partition->writeCluster(currentBuffered, bufferedCluster);
			dirty = 0;
		}
			
		KernelFS::drives[myDrive]->partition->readCluster(t, bufferedCluster);
		currentBuffered = t;
	}

	return 1;
}

BytesCnt KernelFile::filePos()
{
	return cursorPosition;
}

char KernelFile::eof()
{
	if (cursorPosition == fileSize)return 2; //kraj fajla
	if (cursorPosition < 0 || fileSize < 0)return 1; //greska
	if (cursorPosition < fileSize)return 0; //nije kraj fajla
}

BytesCnt KernelFile::getFileSize()
{
	return fileSize;
}

char KernelFile::truncate()
{
	if (cursorPosition == fileSize || fileSize==0)return 0; //kursor je na kraju fajla

	

		//oslobadjamo klastere
		ClusterNo tmp, t;
		 t= tmp = KernelFS::drives[myDrive]->fat->table[currentBuffered];
		KernelFS::drives[myDrive]->fat->table[currentBuffered] = 0;
		while (KernelFS::drives[myDrive]->fat->table[tmp] != 0)
		{
			tmp = KernelFS::drives[myDrive]->fat->table[tmp];
		}
		KernelFS::drives[myDrive]->fat->table[tmp] = KernelFS::drives[myDrive]->freeClusterRoot;
		KernelFS::drives[myDrive]->freeClusterRoot = t;
		fileSize = cursorPosition;
		KernelFS::drives[myDrive]->updateClusters();
		return 1;
	
}