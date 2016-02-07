#ifndef THREAD_HELPER_H
#define THREAD_HELPER_H

#define _WIN32_WINNT 0x0601

#include "Windows.h"
#include "stdio.h"

class CThreadHelper
{
public:
	struct ROUTINE_WRAPPER_INFO
	{
		LPTHREAD_START_ROUTINE lpStartAddress;
		LPVOID lpParameter;
		DWORD nThreadID;
	};

	CThreadHelper();
	~CThreadHelper();

	DWORD RoutineWrapper( LPVOID lpParameter );

	void MultithreadedExecute( LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD nThreads = 0 );
};

#endif //#ifndef THREAD_HELPER_H
