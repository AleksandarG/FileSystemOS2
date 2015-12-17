#pragma once
#include <windows.h>

class FileMonitor{
	HANDLE mutexF;
	
public:
	FileMonitor() {
		
		mutexF = CreateSemaphore(NULL, 1, 1, NULL);
	}

	~FileMonitor(){
		
		if (mutexF) { CloseHandle(mutexF); }
	}

	void startUsing(){
		WaitForSingleObject(mutexF, INFINITE);
	}

	void stopUsing(){
		ReleaseSemaphore(mutexF, 1, NULL);
	}

};
