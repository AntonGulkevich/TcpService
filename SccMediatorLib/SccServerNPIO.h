#pragma once
#define WIN32_LEAN_AND_MEAN

#define OP_READ     0
#define OP_WRITE    1

#define MAX_BUFFER_LEN 256
#define WAIT_TIMEOUT_INTERVAL 30

#include <winsock2.h>
#include <Windows.h> 
#include <tchar.h>
#include <Ws2tcpip.h>
#include <thread>
#include <vector>

class ClientContext  //To store and manage client related information
{
private:

	OVERLAPPED        *m_pol;
	WSABUF            *m_pwbuf;

	INT               m_nTotalBytes;
	INT               m_nSentBytes;

	SOCKET            m_Socket;  //accepted socket
	INT               m_nOpCode; //will be used by the worker thread to decide what operation to perform
	CHAR              m_szBuffer[MAX_BUFFER_LEN];

public:

	//Get/Set calls
	VOID SetOpCode(INT n)
	{
		m_nOpCode = n;
	}

	INT GetOpCode() const
	{
		return m_nOpCode;
	}

	VOID SetTotalBytes(INT n)
	{
		m_nTotalBytes = n;
	}

	INT GetTotalBytes() const
	{
		return m_nTotalBytes;
	}

	VOID SetSentBytes(INT n)
	{
		m_nSentBytes = n;
	}

	VOID IncrSentBytes(INT n)
	{
		m_nSentBytes += n;
	}

	INT GetSentBytes() const
	{
		return m_nSentBytes;
	}

	VOID SetSocket(SOCKET s)
	{
		m_Socket = s;
	}

	SOCKET GetSocket() const
	{
		return m_Socket;
	}

	auto SetBuffer(PCHAR szBuffer, UINT sizeBuffer)
	{
		return memcpy_s(m_szBuffer, MAX_BUFFER_LEN,  szBuffer, sizeBuffer);
	}

	auto GetBuffer(PCHAR szBuffer, UINT sizeBuffer) const
	{
		return memcpy_s(szBuffer, sizeBuffer, m_szBuffer, MAX_BUFFER_LEN);
	}

	VOID ZeroBuffer()
	{
		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);
	}

	VOID SetWSABUFLength(INT nLength) const
	{
		m_pwbuf->len = nLength;
	}

	INT GetWSABUFLength() const
	{
		return m_pwbuf->len;
	}

	WSABUF* GetWSABUFPtr() const
	{
		return m_pwbuf;
	}

	OVERLAPPED* GetOVERLAPPEDPtr() const
	{
		return m_pol;
	}

	VOID ResetWSABUF()
	{
		ZeroBuffer();
		m_pwbuf->buf = m_szBuffer;
		m_pwbuf->len = MAX_BUFFER_LEN;
	}

	//Constructor
	ClientContext()
	{
		m_pol = new OVERLAPPED;
		m_pwbuf = new WSABUF;

		ZeroMemory(m_pol, sizeof(OVERLAPPED));

		m_Socket = SOCKET_ERROR;

		ZeroMemory(m_szBuffer, MAX_BUFFER_LEN);

		m_pwbuf->buf = m_szBuffer;
		m_pwbuf->len = MAX_BUFFER_LEN;

		m_nOpCode = 0;
		m_nTotalBytes = 0;
		m_nSentBytes = 0;
	}

	//destructor
	~ClientContext()
	{
		//Wait for the pending operations to complete
		while (!HasOverlappedIoCompleted(m_pol))
		{
			Sleep(0);
		}

		closesocket(m_Socket);

		//Cleanup
		delete m_pol;
		delete m_pwbuf;
	}
};


class SccServerNPIO
{
	USHORT ushort_port_number;
	INT g_nThreads;

	HANDLE g_hShutdownEvent;
	HANDLE g_hIOCompletionPort;
	WSAEVENT g_hAcceptEvent;

	std::vector<ClientContext *> g_ClientContext;
	std::vector<std::thread> workerThreadsVector;
	std::thread acceptThread;
	CRITICAL_SECTION g_csClientList; 

	SOCKET listenSocket;
	SOCKADDR_IN serverAddress;

	bool InitializeIOCP();
	bool Initialize();
	bool DeInitialize();

	DWORD WINAPI WorkerThread(LPVOID lpParam);
	void AcceptThread();
	void AcceptConnection();
	bool AssociateWithIOCP(ClientContext* pClientContext);

public:
	void AddToClientList(ClientContext* pClientContext);

	// Allow to remove one single client out of the list
	void RemoveFromClientListAndFreeMemory(ClientContext* pClientContext);

	// Clean up the list, this function will be executed at the time of shutdown
	void CleanClientList();
	/*Returns the number of concurrent threads supported by the implementation. 
	 *The value should be considered only a hint.*/
	static UINT GetHardwareConcurrency() noexcept;

	bool Stop();
	bool Start();


	SccServerNPIO();
	~SccServerNPIO();
};

