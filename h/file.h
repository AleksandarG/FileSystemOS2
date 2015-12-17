// File: file.h 
#pragma once
#include "fs.h"

class KernelFile;

class File
{
public:

	char write(BytesCnt, char* buffer);

	BytesCnt read(BytesCnt, char* buffer);

	char seek(BytesCnt);

	BytesCnt filePos();

	char eof();

	BytesCnt getFileSize();

	char truncate(); 

	~File(); //zatvaranje fajla 

private:
	friend class FS;

	friend class KernelFS;

	File(); //objekat fajla se moze kreirati samo otvaranjem 

	KernelFile *myImpl;
};