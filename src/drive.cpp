#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include "drive.h"
#include "fatTable.h"
#include "fs.h"
#include <Windows.h>

#include "list.h"


using namespace std;

Drive::Drive(Partition *part, char let) 
{
	//fatSize = partition->getNumOfClusters();
	partition = part;
	letter = let;
	
	char* cluster = new char[ClusterSize];
	partition->readCluster(0, cluster);
	memcpy(&freeClusterRoot, cluster, 4);
	fatSize = partition->getNumOfClusters();
	
	memcpy(&rootDirectory, cluster+8, 4);
	memcpy(&rootDirectorySize, cluster+12, 4);

	restrictedClusters = (fatSize * 4 + 16) / ClusterSize + 1;

	
	char* buffer = new char[restrictedClusters* ClusterSize];
	memcpy(buffer, cluster + 16, 2032);  //512*4

	

	for (int i = 1; i < restrictedClusters; i++)
	{
		partition->readCluster(i, cluster);
		memcpy(buffer + 2032 + 2048 * (i-1), cluster, ClusterSize);
	
	}

	fat = new FatTable(fatSize, buffer);
	fileOpenPermission = 1;
	numOfOpenedFiles = 0;
	safeToFormat = CreateSemaphore(NULL, 1, 1, NULL);
	
	delete[] cluster;
	delete[] buffer;

}

void Drive::format()
{
	fileOpenPermission = 0;
	WaitForSingleObject(safeToFormat, INFINITE);

	fatSize = partition->getNumOfClusters();
	restrictedClusters = (fatSize * 4 + 16) / ClusterSize + 1;

	fat->formatTable(fatSize, restrictedClusters);

	freeClusterRoot = restrictedClusters; // pocetak i root su isti, ////jer je root prazan
	rootDirectory = restrictedClusters; // treba da se proveri dal odgovara tako
	rootDirectorySize = 0; // broj ulaza u root dir
	fileOpenPermission = 1;
	numOfOpenedFiles = 0;
	updateClusters();
	ReleaseSemaphore(safeToFormat, 1, NULL);
}

void Drive::updateClusters()
{ 
	char* buffer = new char[restrictedClusters* ClusterSize];
	memcpy(buffer, &freeClusterRoot, 4);
	memcpy(buffer + 4, &fatSize, 4);
	memcpy(buffer + 8, &rootDirectory, 4);
	memcpy(buffer + 12, &rootDirectorySize, 4);

	
	memcpy(buffer + 16, fat->table, fatSize*sizeof(long ));

	char* cluster = new char[ClusterSize];
	for (int i = 0; i < restrictedClusters; i++)
	{
		memcpy(cluster, buffer + 2048 * i, ClusterSize);
		partition->writeCluster(i, cluster);
	}
	delete[] buffer;
	delete[] cluster;
}

pair<int, int> Drive::doesExist(string &fname, int mode, Entry *root)
{
	char *buffer = new char[ClusterSize];
	int counter = 0, i=0;
	int currentClusterNum = 0;
	if (root != NULL)
	{
		partition->readCluster(root->firstCluster, buffer);
		counter = root->size;
		currentClusterNum = root->firstCluster;
	}
	else
	{
		partition->readCluster(rootDirectory, buffer);
		counter = rootDirectorySize;
		currentClusterNum = rootDirectory;
	}
	
	
	Entry *e = new Entry();

	char *pch=NULL;
	char* context = NULL;
	pch = strtok_s((char*)fname.c_str(), "\\", &context);

	while (pch != NULL)
	{
		if (counter == 0)break;
		while (i < counter)
		{
			readEntry(buffer, i, *e);
			if (strcmp(pch, e->name) == 0)
			{
				pch = strtok_s(NULL, "\\", &context);
				if (pch == NULL)
				{
					pair<ClusterNo, int> p;
					if (mode == 1)
					{
						p = make_pair(currentClusterNum, i);
					}
					else
					{
						p = make_pair(e->firstCluster, e->size);
					}
					
					delete e;
					delete []buffer;
					return p;
				}
				else
				{
					partition->readCluster(e->firstCluster, buffer);
					counter = e->size;
					i = 0;
					currentClusterNum = e->firstCluster;
					break;
				}
			}
			i++;
			if (i == 102)
			{
				i = 0;
				counter -= 102;
				currentClusterNum = fat->table[currentClusterNum];
				partition->readCluster(currentClusterNum, buffer);
			}
			
		}
		if (i == counter)
			pch = NULL;
	}
	delete e;
	delete[]buffer;

	pair<ClusterNo, int> p = make_pair(-1, -1);
	return p;


}

pair<int, int> Drive::doesExistFile(string &fName)
{
	pair<int, int> ret = make_pair(-1, -1);
	size_t found = fName.find_last_of("\\");
	string name = fName.substr(found + 1);
	pair<int, int> p;
	if (found!=-1)
		p = doesExist(fName.substr(0, found), 0, NULL);
	else p = make_pair(rootDirectory, rootDirectorySize);

	if (p.first != -1)
	{
		found = name.find_last_of(".");
		if (found == -1)return ret;
		string ext = name.substr(found + 1);
		name = name.substr(0, found);
		char *buffer = new char[ClusterSize];
		ClusterNo currentCluster = p.first;
		partition->readCluster(currentCluster, buffer);
		Entry *e = new Entry();
		int counter = p.second, i=0;
		//if (counter == 0)return ret;
		char* fExt = new char[4];
		while (i < counter)
		{
			readEntry(buffer, i, *e);
			
			strcpy(fExt, e->ext);
			fExt[3] = '\0';
			if (strcmp((char*)name.c_str(), e->name) == 0 && strcmp((char*)ext.c_str(), fExt)==0)
			{
				ret = make_pair(currentCluster, i);
				break;
			}
			i++;
			if (i == 102)
			{
				i = 0;
				counter -= 102;
				currentCluster = fat->table[currentCluster];
				partition->readCluster(currentCluster, buffer);
			}
			
		}
		//for (int m = 0; m < 4; m++)
		//cout << fExt[m] << endl;
		//delete[] fExt;
		delete e;
		delete[]buffer;
	}
	
	return ret;
}

char Drive::createDirectory(string &dirName)
{
	char ret = 1;
	char *pch=NULL;
	char* context = NULL;
	Entry *root = new Entry();
	Entry *parent = new Entry();
	Entry *oldRoot = new Entry();
	int exist = 0;
	root->firstCluster = rootDirectory;
	root->size = rootDirectorySize;
	strcpy(root->name, "glavni");
	oldRoot->firstCluster = rootDirectory;
	oldRoot->size = rootDirectorySize;

	pch = strtok_s((char*)dirName.c_str(), "\\", &context);
	while (pch != NULL )
	{
		pair<ClusterNo, int> par = doesExist(string(pch), 0, root);
		if (par.first==-1)
		{
			memcpy(parent->name, pch, 8);
			parent->attributes =  '2';
			parent->firstCluster = 0;
			parent->size = 0;
			exist = 0;
			makeDirectory(*root, *parent);
			pair<ClusterNo, int> p;
			if (strcmp(root->name, "glavni"))
			{
				p = doesExist(string(root->name), 1, oldRoot);
				char *buffer = new char[ClusterSize];
				partition->readCluster(p.first, buffer);
				if (root->size==1)
					memcpy(buffer + p.second * 20 + 12, &root->firstCluster, 4);
				memcpy(buffer + p.second * 20 + 16, &root->size, 4);
				partition->writeCluster(p.first, buffer);
				delete[] buffer;
				

			}
			else
			{
				rootDirectory = root->firstCluster;
				rootDirectorySize = root->size;
				updateClusters();
			}

		}
		else
		{
			memcpy(parent->name, pch, 8);
			parent->attributes = '2';
			parent->firstCluster = par.first;
			parent->size= par.second;// treba provera
			//oldRoot = root;
			//root = parent;
			exist = 1;
		}
		memcpy(oldRoot, root, 20);
		memcpy(root, parent, 20);
		pch = strtok_s(NULL, "\\", &context);
		if (pch == NULL && exist == 1)
			ret = 0; //vec postoji direktorijum
	}
	delete root;
	delete parent;
	delete oldRoot;
	return ret;
}

void Drive::makeDirectory(Entry &rootDir, Entry &dir)
{
	char *buffer = new char[ClusterSize];
	ClusterNo newCluster;
	if (rootDir.size == 0)
	{
		newCluster= rootDir.firstCluster = this->getFreeCluster();
		
	}
	else 
	{
		ClusterNo tmp = rootDir.firstCluster;
		while (fat->table[tmp] != 0)
		{
			tmp = fat->table[tmp];
		}
		int i = rootDir.size % 102;
		
		

		if (i == 0)
		{
			newCluster = getFreeCluster();
			fat->table[tmp] = newCluster;
			updateClusters();
			partition->readCluster(newCluster, buffer);
			tmp = newCluster;
		}
		else
			partition->readCluster(tmp, buffer);
		memcpy(buffer + i * 20, dir.name, 8);
		memcpy(buffer + i * 20 + 8, dir.ext, 3);
		memcpy(buffer + i * 20 + 11, &dir.attributes, 1);
		memcpy(buffer + i * 20 + 12, &dir.firstCluster, 4);
		memcpy(buffer + i * 20 + 16, &dir.size, 4);
		partition->writeCluster(tmp, buffer);
		delete[] buffer;
		rootDir.size++;
		return;
	}
	partition->readCluster(newCluster, buffer);
	memcpy(buffer, dir.name, 8);
	memcpy(buffer + 8, dir.ext, 3);
	memcpy(buffer + 11, &dir.attributes, 1);
	memcpy(buffer + 12, &dir.firstCluster, 4);
	memcpy(buffer + 16, &dir.size, 4);
	partition->writeCluster(newCluster, buffer);
	delete[] buffer;
	rootDir.size++;

}

void Drive::readEntry(char* buffer, EntryNum n, Entry &e)
{
	memcpy(e.name, buffer + n * 20, 8);
	memcpy(e.ext, buffer + n * 20 + 8, 3);
	memcpy(&e.attributes, buffer + n * 20 + 11, 1);
	memcpy(&e.firstCluster, buffer + n * 20 + 12, 4);
	memcpy(&e.size, buffer + n * 20 + 16, 4);

}
ClusterNo Drive::getFreeCluster()
{
	if (freeClusterRoot == 0)return 0;  //nema slobodnih klastera
	ClusterNo ret = freeClusterRoot;
	
	ClusterNo newFree = fat->table[freeClusterRoot];
	fat->table[freeClusterRoot] = 0;
	freeClusterRoot = newFree;
	updateClusters();
	
	return ret;
}

void Drive::showEntries(string &dirName)
{
	pair<int, int> p = make_pair(rootDirectory, rootDirectorySize);
	if(dirName!="") p= doesExist(dirName, 0, NULL);
	if (p.first != -1)
	{
		int i = 0, count = p.second;
		int currentClusterNum = p.first;
		Entry *e = new Entry();
		char *buffer = new char[ClusterSize];
		partition->readCluster(p.first, buffer);
		cout << "direktorijum sadrzi: \n";
		while (i < count)
		{
			readEntry(buffer, i, *e);
			cout << e->name << " " << e->ext << " " << e->attributes 
				<< " " << e->firstCluster << " " << e->size << endl;

			i++;
			if (i == 102)
			{
				i = 0;
				count -= 102;
				currentClusterNum = fat->table[currentClusterNum];
				partition->readCluster(currentClusterNum, buffer);
			}
		}
		delete[] buffer;
		delete e;
	}
}

char Drive::readDir(string &dirname, EntryNum n, Entry &e){
	pair<int, int> p;
	if (dirname == "")p = make_pair(rootDirectory, rootDirectorySize);
	else p = doesExist(dirname, 0, NULL);

	if (p.first == -1)return 0;  //pogresna putanja
	else if (p.second <= n) return 2; //prekoracenje
	else
	{
		int clusterNumber = n / 102;
		int	positionInCluster = n % 102;
		char* buffer = new char[ClusterSize];
		int tmp = p.first, i=0;
		while (i != clusterNumber)
		{
			tmp = fat->table[tmp];
			i++;
		}
		partition->readCluster(tmp, buffer);
		memcpy(&e, buffer + positionInCluster * 20, 20);
		delete[] buffer;
	}
	return 1;
}

char Drive::deleteDir(string &dirName)
{
	pair<int, int> p;
	if (dirName == "")return 0;  //ne moze da se brise root
	else p = doesExist(string(dirName), 0, NULL);

	if (p.first == -1 || p.second!=0)return 0;  //pogresna putanja ili direktorijum nije prazan
	
	char *pch = NULL;
	char *context = NULL;
	Entry *root = new Entry();
	Entry *parent = new Entry();
	Entry *oldRoot = new Entry();
	int exist = 0;
	root->firstCluster = rootDirectory;
	root->size = rootDirectorySize;
	strcpy(root->name, "glavni");
	//oldRoot->firstCluster = rootDirectory;
	//oldRoot->size = rootDirectorySize;

	pch = strtok_s((char*)dirName.c_str(), "\\", &context);
	
	while (pch!=NULL)
	{
		pair<ClusterNo, int> par = doesExist(string(pch), 0, root);
		memcpy(parent->name, pch, 8);
		parent->attributes = '2';
		parent->firstCluster = par.first;
		parent->size = par.second;
		if ((pch = strtok_s(NULL, "\\", &context)) != NULL)
		{
			memcpy(oldRoot, root, 20);
			memcpy(root, parent, 20);
		}
		
		//pch = strtok_s(NULL, "\\", &context);
	} 

	p = doesExist(string(parent->name), 1, root);
	
	
	char* buffer = new char[ClusterSize];
	int tmp = root->firstCluster, i = 0;
	int tmpRoot = root->firstCluster;
	int numOfClusters = root->size / 102;
	if (root->size % 102 == 0)numOfClusters--;
	while (i<numOfClusters)
	{
		tmpRoot = tmp;
		tmp = fat->table[tmp];
		i++;
	}
	
	int positionOfLastEntry = (root->size-1) % 102 ;
	if (p.first == tmp && p.second == positionOfLastEntry)
	{
		//brisemo poslednji entry u tom klasteru
		
	}
	else
	{//prebacujemo poslednji entry na mesto ovog sto brisemo
		partition->readCluster(tmp, buffer);
		Entry *e = new Entry();
		memcpy(e, buffer + positionOfLastEntry * 20, 20);
		if (tmp != p.first)
			partition->readCluster(p.first, buffer);
		memcpy(buffer + p.second*20, e, 20);
		partition->writeCluster(p.first, buffer);
		delete e;
	}
	root->size--;
	if (root->size % 102 == 0)
	{
		if (tmpRoot != tmp)
			fat->table[tmpRoot] = 0; //treba da se updatuju clusteri, al to se radi 3 reda nize
		else
			root->firstCluster = 0; //znaci da mu je size=0 i da ne treba da zauzima nijedan cluster
		setClusterFree(tmp);
	}

	
	if (strcmp(root->name, "glavni"))
	{
		p = doesExist(string(root->name), 1, oldRoot);
		
		partition->readCluster(p.first, buffer);
		if (root->size == 0)
			memcpy(buffer + p.second * 20 + 12, &root->firstCluster, 4);
		memcpy(buffer + p.second * 20 + 16, &root->size, 4);
		partition->writeCluster(p.first, buffer);

	}
	else
	{
		rootDirectory = root->firstCluster;
		rootDirectorySize = root->size;
		updateClusters();
	}	
	
	delete[] buffer;
	delete root;
	delete parent;
	delete oldRoot;
	delete pch;
	delete context;
	
	return 1;
}

void Drive::setClusterFree(ClusterNo n)
{
	fat->table[n] = freeClusterRoot;
	freeClusterRoot = n;
	updateClusters();
}

Partition* Drive::getPartition() const
{
	return partition;
}

char Drive::getLetter() const
{
	return letter;
}

void Drive::printD()
{
	cout << letter << " " << freeClusterRoot << " " << fatSize << " " << rootDirectory << " " << rootDirectorySize << endl;
}

KernelFile* Drive::openFile(string &fName, char mode)
{
	
	if (fileOpenPermission == 0)return NULL;  //nije dozvoljeno otvaranje fajlova, jer je uradjen unmount ili format koji ceka

	List::add(string(fName));

	pair<int, int> p;
	KernelFile *newKernelFile = NULL;
	if (mode == 'r')
	{
		p = doesExistFile(fName);
		if (p.first == -1)
		{
			
			List::remove(string(fName));
			return NULL;
		}
		else
		{
			char *buffer = new char[ClusterSize];
			partition->readCluster(p.first, buffer);
			Entry *e = new Entry();
			readEntry(buffer, p.second, *e);
			newKernelFile = new KernelFile(fName, 0, e->size, mode, e->firstCluster, letter);
			
			
			delete[] buffer;
			delete e;
		}
	}
	else if (mode == 'w')
	{
		p = doesExistFile(fName);
		
		if(p.first != -1)
		{
			deleteFile(string(fName));
		}

		else
		{
		}
		

		Entry *e = new Entry();
		createFile(string(fName), *e);
		
		newKernelFile = new KernelFile(fName, 0, e->size, mode, e->firstCluster, letter);
		
		delete e;

	}
	else if (mode == 'a')
	{
		p = doesExistFile(fName);
		if (p.first == -1)
		{
			
			List::remove(string(fName));
			return NULL;
			
		}
		else
		{
			char *buffer = new char[ClusterSize];
			partition->readCluster(p.first, buffer);
			Entry *e = new Entry();
			readEntry(buffer, p.second, *e);
			delete[] buffer;
			newKernelFile = new KernelFile(fName, e->size, e->size, mode, e->firstCluster, letter);
			
			delete e;
		}

	}
	else
	{
		List::remove(string(fName));
		return NULL; //pogresan mod
	}
		
	
	
	return newKernelFile;
	
}

void Drive::createFile(string& fName, Entry &e)
{
	size_t found = fName.find_last_of(".");
	string fileExt = fName.substr(found + 1);
	fName = fName.substr(0, found);
	found = fName.find_last_of("\\");
	string fileName = fName.substr(found + 1);
	fName = fName.substr(0, found);
	found = fName.find_last_of("\\");
	string parent="glavni";
	string root="glavni";
	if (fName!=fileName)
	{
		found = fName.find_last_of("\\");
		parent = fName.substr(found + 1);
		fName = fName.substr(0, found);
		
		if (parent != fName)
		{
			found = fName.find_last_of("\\");
			root = fName.substr(found + 1);
		}
		//fName = fName.substr(0, found-1);
	}	
	

	Entry *rootDir = new Entry();
	Entry *parentDir = new Entry();
	Entry *newEntry = new Entry();
	if (root != "glavni")
	{

		pair<int, int> p = doesExist(fName, 0, NULL);
		rootDir->firstCluster = p.first;
		rootDir->size = p.second;
		//strcpy(rootDir->name, (char*)root.c_str());
		
	}
	else
	{
		rootDir->firstCluster = rootDirectory;
		rootDir->size = rootDirectorySize;
	}

	strcpy(newEntry->name, (char*)fileName.c_str());
	strcpy(newEntry->ext, (char*)fileExt.c_str());
	newEntry->attributes = '1';
	newEntry->firstCluster = 0;
	newEntry->size = 0;

	if (parent != "glavni")
	{
		pair<int, int> p = doesExist(parent, 0, rootDir);
		parentDir->firstCluster = p.first;
		parentDir->size = p.second;
		makeDirectory(*parentDir, *newEntry);

		p = doesExist(parent, 1, rootDir);
		char *buffer = new char[ClusterSize];
		partition->readCluster(p.first, buffer);
		if (parentDir->size == 1)
			memcpy(buffer + p.second * 20 + 12, &parentDir->firstCluster, 4);
		memcpy(buffer + p.second * 20 + 16, &parentDir->size, 4);
		partition->writeCluster(p.first, buffer);
		delete[] buffer;
	}
	else
	{
		makeDirectory(*rootDir, *newEntry);
		rootDirectory = rootDir->firstCluster;
		rootDirectorySize = rootDir->size;
		updateClusters();
	}
	
	delete rootDir;
	delete parentDir;
	memcpy(&e, newEntry, 20);
	delete newEntry;
}

char Drive::deleteFile(string &fName)
{
	//prvo provera da li je u openedFiles
	//onda da li postoji na disku
	pair<int, int> par=doesExistFile(fName);
	bool isOpen = List::isOpen(string(fName));				
	if (isOpen || par.first == -1)
		return 0;
	//if (par.first == -1)return '0';
	char *buff = new char[ClusterSize];
	partition->readCluster(par.first, buff);
	Entry *e = new Entry();
	readEntry(buff, par.second, *e);

	if (e->size > 0)
	{
		 //oslobadjamo klastere
		ClusterNo tmp = e->firstCluster;
		while (fat->table[tmp] != 0)
		{
			tmp = fat->table[tmp];
		}
		fat->table[tmp] = freeClusterRoot;
		freeClusterRoot = e->firstCluster;
	}

	size_t found = fName.find_last_of(".");
	string fileExt = fName.substr(found + 1);
	fName = fName.substr(0, found);
	found = fName.find_last_of("\\");
	string fileName = fName.substr(found + 1);
	fName = fName.substr(0, found);
	
	string parent = "glavni";
	string root = "glavni";
	if (fName!=fileName)
	{
		found = fName.find_last_of("\\");
		parent = fName.substr(found + 1);
		fName = fName.substr(0, found);
		
		if (parent != fName)
		{
			found = fName.find_last_of("\\");
			root = fName.substr(found + 1);
		}
			

		//fName = fName.substr(0, found - 1);
	}


	Entry *rootDir = new Entry();
	Entry *parentDir = new Entry();
	//Entry *newEntry = new Entry();
	if (root != "glavni")
	{

		pair<int, int> p = doesExist(fName, 0, NULL);
		rootDir->firstCluster = p.first;
		rootDir->size = p.second;
		//strcpy(rootDir->name, (char*)root.c_str());

	}
	else
	{
		rootDir->firstCluster = rootDirectory;
		rootDir->size = rootDirectorySize;
	}


	if (parent != "glavni")
	{
		pair<int, int> p = doesExist(parent, 0, rootDir);
		parentDir->firstCluster = p.first;
		parentDir->size = p.second;
	}
	else
	{
		parentDir->firstCluster = rootDirectory;
		parentDir->size = rootDirectorySize;
	}


	//p = doesExist(string(parent->name), 1, root);


	char* buffer = new char[ClusterSize];
	int tmp = parentDir->firstCluster, i = 0;
	int tmpRoot = parentDir->firstCluster;
	int numOfClusters = parentDir->size / 102;
	if (parentDir->size % 102 == 0)numOfClusters--;
	while (i<numOfClusters)
	{
		tmpRoot = tmp;
		tmp = fat->table[tmp];
		i++;
	}

	int positionOfLastEntry = (parentDir->size - 1) % 102;
	if (par.first == tmp && par.second == positionOfLastEntry)
	{
		//brisemo poslednji entry u tom klasteru

	}
	else
	{//prebacujemo poslednji entry na mesto ovog sto brisemo
		partition->readCluster(tmp, buffer);
		Entry *e = new Entry();
		memcpy(e, buffer + positionOfLastEntry * 20, 20);
		if (tmp != par.first)
			partition->readCluster(par.first, buffer);
		memcpy(buffer + par.second * 20, e, 20);
		partition->writeCluster(par.first, buffer);
		delete e;
	}
	parentDir->size--;
	if (parentDir->size % 102 == 0)
	{
		if (tmpRoot != tmp)
			fat->table[tmpRoot] = 0; //treba da se updatuju clusteri, al to se radi 3 reda nize
		else
			parentDir->firstCluster = 0; //znaci da mu je size=0 i da ne treba da zauzima nijedan cluster
		setClusterFree(tmp);
	}


	if (parent!= "glavni")
	{
		par = doesExist(parent, 1, rootDir);
		

		partition->readCluster(par.first, buffer);
		if (parentDir->size == 0)
			memcpy(buffer + par.second * 20 + 12, &parentDir->firstCluster, 4);
		memcpy(buffer + par.second * 20 + 16, &parentDir->size, 4);
		partition->writeCluster(par.first, buffer);

	}
	else
	{
		rootDirectory = parentDir->firstCluster;
		rootDirectorySize = parentDir->size;
		updateClusters();
	}
	delete[] buffer;
	delete rootDir;
	delete parentDir;
	//delete newEntry;
	return 1;
}

Drive::~Drive()
{
	fileOpenPermission = 0;
	WaitForSingleObject(safeToFormat, INFINITE);
	if (safeToFormat) 
		CloseHandle(safeToFormat);
	updateClusters();
	//partition = 0;
	//delete partition;
	delete fat;
}
