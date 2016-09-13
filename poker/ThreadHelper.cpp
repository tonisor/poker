#include "ThreadHelper.h"

CThreadHelper::CThreadHelper()
{
}

CThreadHelper::~CThreadHelper()
{
}

DWORD CThreadHelper::RoutineWrapper( LPVOID lpParameter )
{
	const auto& info = *reinterpret_cast<ROUTINE_WRAPPER_INFO*>(lpParameter);

	ROUTINE_WRAPPER wrapper;
	wrapper.lpParameter = info.lpParameter;
	wrapper.nThreadID = info.nThreadID;

	(*info.lpStartAddress)(reinterpret_cast<LPVOID>(&wrapper));

	return 0;
}

void CThreadHelper::MultithreadedExecute( LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD nThreads/* = 0*/ )
{
	if( nThreads == 0 )
		nThreads = GetActiveProcessorCount( ALL_PROCESSOR_GROUPS );

	auto t0 = GetTickCount();

	auto* pHandles = new HANDLE[nThreads];
	auto* pRoutines = new ROUTINE_WRAPPER_INFO[nThreads];

	for( DWORD kThread = 0; kThread < nThreads; kThread++ )
	{
		pRoutines[kThread].lpStartAddress = lpStartAddress;
		pRoutines[kThread].lpParameter = lpParameter;
		pRoutines[kThread].nThreadID = kThread;

		pHandles[kThread] = CreateThread( NULL, 0, RoutineWrapper, reinterpret_cast<LPVOID>(&pRoutines[kThread]), 0, NULL );
		if( pHandles[kThread] == NULL )
		{
			exit(-1);
		}
	}

	WaitForMultipleObjects( nThreads, pHandles, TRUE, INFINITE );

	delete[] pHandles;
	delete[] pRoutines;

	//printf( "%d\n", GetTickCount() - t0 );
}
