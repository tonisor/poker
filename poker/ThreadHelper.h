#ifndef THREAD_HELPER_H
#define THREAD_HELPER_H

#include "Windows.h"

class CThreadHelper
{
public:
	struct ROUTINE_WRAPPER
	{
		LPVOID lpParameter;
		DWORD nThreadID;
	};

	CThreadHelper();
	~CThreadHelper();

	void MultithreadedExecute( LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD nThreads = 0 );

private:
	struct ROUTINE_WRAPPER_INFO
	{
		LPTHREAD_START_ROUTINE lpStartAddress;
		LPVOID lpParameter;
		DWORD nThreadID;
	};

	static DWORD WINAPI RoutineWrapper(LPVOID lpParameter);
};

#endif //#ifndef THREAD_HELPER_H
