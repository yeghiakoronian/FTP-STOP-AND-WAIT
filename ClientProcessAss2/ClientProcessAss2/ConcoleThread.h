#pragma once
class ConcoleThread
{
public:
	HANDLE m_thread;
	
	static DWORD WINAPI run( LPVOID lpParam );
	ConcoleThread(void);
	~ConcoleThread(void);

	void startProcessing();
};

