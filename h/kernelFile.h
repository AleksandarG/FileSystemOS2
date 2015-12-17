// File: kernelFile.h
#pragma once

#include "fs.h"
#include "part.h"
#include <string>
#include <Windows.h>

using namespace std;
class Drive;

class KernelFile
{
public:
	KernelFile(string name, BytesCnt cursor, BytesCnt size, char m, ClusterNo first, char drive);
	~KernelFile();

	char write(BytesCnt, char* buffer);

	BytesCnt read(BytesCnt, char* buffer);

	char seek(BytesCnt);

	BytesCnt filePos();

	char eof();

	BytesCnt getFileSize();

	char truncate();



private:


	string fileName;		
	char myDrive;
	BytesCnt fileSize;				
	BytesCnt cursorPosition;			
	char mode;					
	ClusterNo firstCluster;			

	int dirty;
	char *bufferedCluster;
	int currentBuffered;
	int positionInCluster;
	


};

