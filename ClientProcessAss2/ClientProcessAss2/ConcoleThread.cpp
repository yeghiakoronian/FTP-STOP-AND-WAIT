#include "stdafx.h"
#include "ConcoleThread.h"


ConcoleThread::ConcoleThread(void)
{

	m_thread = CreateThread( 
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ConcoleThread::run,       // thread function name
		this,          // argument to thread function 
		0,                      // use default creation flags 
		NULL);   // returns the thread identifier 

}

DWORD WINAPI  ConcoleThread::run( LPVOID lpParam ){
	return 0;
}

ConcoleThread::~ConcoleThread(void)
{
}

void ConcoleThread::startProcessing(){
}
