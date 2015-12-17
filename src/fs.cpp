#include "fs.h"
#include "kernelFS.h"


KernelFS *FS::myImpl = new KernelFS();

FS::~FS()
{
	delete FS::myImpl;
}

char FS::mount(Partition* partition)
{
	return FS::myImpl->mount(partition);
}

char FS::unmount(char part)
{
	return FS::myImpl->unmount(part);
}

char FS::format(char part)
{
	return FS::myImpl->format(part);
}

char FS::doesExist(char* fname)
{
	return FS::myImpl->doesExist(fname);
}

File* FS::open(char* fname, char mode)
{
	return FS::myImpl->open(fname, mode);
}

char FS::deleteFile(char* fname)
{
	return FS::myImpl->deleteFile(fname);
}

char FS::createDir(char* dirname)
{
	return FS::myImpl->createDir(dirname);
}

char FS::deleteDir(char* dirname)
{
	return FS::myImpl->deleteDir(dirname);
}

char FS::readDir(char* dirname, EntryNum n, Entry &e)
{
	return FS::myImpl->readDir(dirname, n, e);
}