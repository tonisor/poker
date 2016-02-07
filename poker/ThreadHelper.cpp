#include "ThreadHelper.h"

CThreadHelper::CThreadHelper()
{
}

CThreadHelper::~CThreadHelper()
{
}

DWORD CThreadHelper::RoutineWrapper( LPVOID lpParameter )
{
	ROUTINE_WRAPPER_INFO* info = (ROUTINE_WRAPPER_INFO*)lpParameter;

	(*info->lpStartAddress)( info->lpParameter );

	return 0;
}

void CThreadHelper::MultithreadedExecute( LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD nThreads/* = 0*/ )
{
	if( nThreads == 0 )
		nThreads = GetActiveProcessorCount( ALL_PROCESSOR_GROUPS );

	DWORD t0 = GetTickCount();

	HANDLE* pHandles = new HANDLE[nThreads];

	for( DWORD kThread = 0; kThread < nThreads; kThread++ )
	{
		pHandles[kThread] = CreateThread( NULL, 0, lpStartAddress, lpParameter, 0, NULL );
		if( pHandles[kThread] == NULL )
		{
			exit(-1);
		}
	}

	WaitForMultipleObjects( nThreads, pHandles, TRUE, INFINITE );

	//printf( "%d\n", GetTickCount() - t0 );
}
