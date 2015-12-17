// File: fs.h
#pragma once

typedef unsigned long BytesCnt;
typedef unsigned long EntryNum;

const unsigned int FNAMELEN = 8;
const unsigned int FEXTLEN = 3;

struct Entry{
	char name[FNAMELEN];
	char ext[FEXTLEN];
	char attributes;
	unsigned long firstCluster;
	unsigned long size;
};

class KernelFS;
class Partition;
class File;

class FS
{
public:
	~FS();

	static char mount(Partition* partition); // montira particiju
											// vraca dodeljeno slovo
											// ili 0 u slucaju neuspeha

	static char unmount(char part);  // demontira particiju oznacenu datim
			// slovom vara 0 u slucaju neuspeha ili 1 u slucaju uspeha
	static char format(char part); //formatira particiju sa datim slovom	;
	//	vraca 0 u slucaju neuspeha ili 1 u slucaju uspeha

	static char doesExist(char* fname); // argument je naziv fajla 
			// zadat apsolutnom putanjom

	static File* open(char* fname, char mode); // prvi argument je naziv
		// fajla zadat apsolutnom putanjom, drugi modalitet

	static char deleteFile(char* fname); // argument je naziv
		// fajla zadat apsolutnom putanjom

	static char createDir(char* dirname); // argument je naziv 
		// direktorijuma zadat apsolutnom putanjom

	static char deleteDir(char* dirname); // argument je naziv 
		// direktorijuma zadat apsolutnom putanjom

	static char readDir(char* dirname, EntryNum n, Entry &e);
		// prvim argumentom se zadaje apsolutna putanja, drugim redni broj
		// ulaza koji se cita, treci argument je adresa na kojoj se smesta 
		// procitani ulaz

protected:
	FS();
	static KernelFS *myImpl;
};

