#include "SccServerNPIO.h"

UINT SccServerNPIO::GetHardwareConcurrency() noexcept
{
	return std::thread::hardware_concurrency() * 2;
}

DWORD SccServerNPIO::WorkerThread(LPVOID lpParam)
{
	auto nTreadNo = INT(lpParam);

	void *lpContext = nullptr;
	LPOVERLAPPED pOverlapped = nullptr;
	ClientContext   *pClientContext = nullptr;
	DWORD dwBytesTransfered = 0;
	INT nBytesRecv = 0;
	INT nBytesSent = 0;
	DWORD dwBytes = 0, dwFlags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(g_hShutdownEvent, 0))
	{
		auto bReturn = GetQueuedCompletionStatus(
			g_hIOCompletionPort,
			&dwBytesTransfered,
			reinterpret_cast<LPDWORD>(&lpContext),
			&pOverlapped,
			INFINITE);
		if (lpContext == nullptr) return 0; // fatal here

		pClientContext = static_cast<ClientContext *>(lpContext);

		if ((FALSE == bReturn) || ((TRUE == bReturn) && (0 == dwBytesTransfered)))
		{
			//Client connection gone, remove it.
			RemoveFromClientListAndFreeMemory(pClientContext);
			continue;
		}
		WSABUF *p_wbuf = pClientContext->GetWSABUFPtr();
		OVERLAPPED *p_ol = pClientContext->GetOVERLAPPEDPtr();
		switch (pClientContext->GetOpCode())
		{
		case OP_READ:

			pClientContext->IncrSentBytes(dwBytesTransfered);

			//Write operation was finished, see if all the data was sent.
			//Else post another write.
			if (pClientContext->GetSentBytes() < pClientContext->GetTotalBytes())
			{
				pClientContext->SetOpCode(OP_READ);

				p_wbuf->buf += pClientContext->GetSentBytes();
				p_wbuf->len = pClientContext->GetTotalBytes() - pClientContext->GetSentBytes();

				dwFlags = 0;

				//Overlapped send
				nBytesSent = WSASend(pClientContext->GetSocket(), p_wbuf, 1,
					&dwBytes, dwFlags, p_ol, NULL);

				if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					//Let's not work with this client
					RemoveFromClientListAndFreeMemory(pClientContext);
				}
			}
			else
			{
				//Once the data is successfully received, we will print it.
				pClientContext->SetOpCode(OP_WRITE);
				pClientContext->ResetWSABUF();

				dwFlags = 0;

				//Get the data.
				nBytesRecv = WSARecv(pClientContext->GetSocket(), p_wbuf, 1,
					&dwBytes, &dwFlags, p_ol, NULL);

				if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					//"Thread Error occurred while executing WSARecv()."
					//Let's not work with this client
					RemoveFromClientListAndFreeMemory(pClientContext);
				}
			}

			break;

		case OP_WRITE:

			char szBuffer[MAX_BUFFER_LEN];

			pClientContext->GetBuffer(szBuffer, MAX_BUFFER_LEN);

			//WriteToConsole("\nThread %d: The following message was received: %s", nThreadNo, szBuffer);

			//Send the message back to the client.
			pClientContext->SetOpCode(OP_READ);


			pClientContext->SetTotalBytes(dwBytesTransfered);
			pClientContext->SetSentBytes(0);

			p_wbuf->len = dwBytesTransfered;

			dwFlags = 0;

			//Overlapped send
			nBytesSent = WSASend(pClientContext->GetSocket(), p_wbuf, 1,
				&dwBytes, dwFlags, p_ol, NULL);

			if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
			{
				//WriteToConsole("\nThread %d: Error occurred while executing WSASend().", nThreadNo);

				//Let's not work with this client
				RemoveFromClientListAndFreeMemory(pClientContext);
			}

			break;

		default:
			//We should never be reaching here, under normal circumstances.
			break;
		} // switch

	}

	return 0;
}

void SccServerNPIO::AcceptThread()
{
	WSANETWORKEVENTS WSAEvents;
	//Accept thread will be around to look for accept event, until a Shutdown event is not Signaled.
	while (WAIT_OBJECT_0 != WaitForSingleObject(g_hShutdownEvent, 0))
	{
		if (WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents(1, &g_hAcceptEvent, FALSE, WAIT_TIMEOUT_INTERVAL, FALSE))
		{
			WSAEnumNetworkEvents(listenSocket, g_hAcceptEvent, &WSAEvents);
			if ((WSAEvents.lNetworkEvents & FD_ACCEPT) && (0 == WSAEvents.iErrorCode[FD_ACCEPT_BIT]))
				AcceptConnection();
		}
	}
}

void SccServerNPIO::AcceptConnection()
{
	sockaddr_in ClientAddress;
	int nClientLength = sizeof(ClientAddress);

	//Accept remote connection attempt from the client
	auto acceptedSocket = accept(listenSocket, reinterpret_cast<PSOCKADDR>(&ClientAddress), &nClientLength);

	if (acceptedSocket == INVALID_SOCKET)
	{
		//WriteToConsole("\nError occurred while accepting socket: %ld.", WSAGetLastError());
		return;
	}

	//Display Client's IP
	//WriteToConsole("\nClient connected from: %s", inet_ntoa(ClientAddress.sin_addr));

	//Create a new ClientContext for this newly accepted client
	auto pClientContext = new ClientContext;

	pClientContext->SetOpCode(OP_READ);
	pClientContext->SetSocket(acceptedSocket);

	//Store this object
	AddToClientList(pClientContext);

	if (true == AssociateWithIOCP(pClientContext))
	{
		//Once the data is successfully received, we will print it.
		pClientContext->SetOpCode(OP_WRITE);

		auto p_wbuf = pClientContext->GetWSABUFPtr();
		auto p_ol = pClientContext->GetOVERLAPPEDPtr();

		//Get data.
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;

		//Post initial Recv
		//This is a right place to post a initial Recv
		//Posting a initial Recv in WorkerThread will create scalability issues.
		auto nBytesRecv = WSARecv(pClientContext->GetSocket(), p_wbuf, 1,
		                         &dwBytes, &dwFlags, p_ol, nullptr);

		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{
			//WriteToConsole("\nError in Initial Post.");
		}
	}
}

bool SccServerNPIO::AssociateWithIOCP(ClientContext* pClientContext)
{
	//Associate the socket with IOCP
	auto hTemp = CreateIoCompletionPort(reinterpret_cast<HANDLE>(pClientContext->GetSocket()), g_hIOCompletionPort, reinterpret_cast<DWORD>(pClientContext), IGNORE);

	if (hTemp == nullptr)
	{
		//WriteToConsole("\nError occurred while executing CreateIoCompletionPort().");
		RemoveFromClientListAndFreeMemory(pClientContext);
		return false;
	}
	return true;
}

bool SccServerNPIO::InitializeIOCP()
{
	g_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, NULL);
	if (g_hIOCompletionPort == nullptr) return false;

	for (auto i = 0; i < g_nThreads; ++i)
	{
		workerThreadsVector.emplace_back(std::thread(&SccServerNPIO::WorkerThread, this, reinterpret_cast<void *>(i + 1)));
	}

	return true;
}

bool SccServerNPIO::Initialize()
{
	g_nThreads = GetHardwareConcurrency();
	g_hShutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	InitializeCriticalSection(&g_csClientList);

	return InitializeIOCP();

}

bool SccServerNPIO::DeInitialize()
{
	closesocket(listenSocket);
	DeleteCriticalSection(&g_csClientList);
	CloseHandle(g_hIOCompletionPort);
	CloseHandle(g_hShutdownEvent);

	WSACleanup();
	return false;
}

void SccServerNPIO::AddToClientList(ClientContext* pClientContext)
{
	EnterCriticalSection(&g_csClientList);

	//Store these structures in vectors
	g_ClientContext.push_back(pClientContext);

	LeaveCriticalSection(&g_csClientList);
}

void SccServerNPIO::RemoveFromClientListAndFreeMemory(ClientContext* pClientContext)
{
	EnterCriticalSection(&g_csClientList);

	std::vector<ClientContext *>::iterator IterClientContext;

	//Remove the supplied ClientContext from the list and release the memory
	for (IterClientContext = g_ClientContext.begin(); IterClientContext != g_ClientContext.end(); IterClientContext++)
	{
		if (pClientContext == *IterClientContext)
		{
			g_ClientContext.erase(IterClientContext);

			//i/o will be cancelled and socket will be closed by destructor.
			delete pClientContext;
			break;
		}
	}

	LeaveCriticalSection(&g_csClientList);
}

void SccServerNPIO::CleanClientList()
{
	EnterCriticalSection(&g_csClientList);

	std::vector<ClientContext *>::iterator IterClientContext;

	for (IterClientContext = g_ClientContext.begin(); IterClientContext != g_ClientContext.end(); IterClientContext++)
	{
		//i/o will be cancelled and socket will be closed by destructor.
		delete *IterClientContext;
	}

	g_ClientContext.clear();

	LeaveCriticalSection(&g_csClientList);
}

bool SccServerNPIO::Stop()
{
	SetEvent(g_hShutdownEvent);
	if (acceptThread.joinable())
		acceptThread.join();
	for (auto i = 0 ; i < g_nThreads; ++i)
		PostQueuedCompletionStatus(g_hIOCompletionPort, IGNORE, IGNORE, nullptr);

	for (auto it = workerThreadsVector.begin(); it!=workerThreadsVector.end(); ++it)
	{
		if (it->joinable())
			it->join();
	}

	WSACloseEvent(g_hAcceptEvent);
	CleanClientList();

	DeInitialize();

	return false;
}

bool SccServerNPIO::Start()
{
	if (!Initialize()) return false;
	//Overlapped I/O follows the model established in Windows and can be performed only on 
	//sockets created through the WSASocket function 
	listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET) return false;

	ZeroMemory(reinterpret_cast<char *>(&serverAddress), sizeof(serverAddress));

	//Fill up the address structure
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(ushort_port_number);

	if (bind(listenSocket, reinterpret_cast<PSOCKADDR>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
		return DeInitialize();

	if (listen(listenSocket, SOMAXCONN))
		return DeInitialize();

	g_hAcceptEvent = WSACreateEvent();
	if (g_hAcceptEvent == WSA_INVALID_EVENT)
		return DeInitialize();

	if (WSAEventSelect(listenSocket, g_hAcceptEvent, FD_ACCEPT) == SOCKET_ERROR)
		return DeInitialize();

	acceptThread = std::thread(&SccServerNPIO::AcceptThread, this);

	return true;

}

SccServerNPIO::SccServerNPIO() :
	ushort_port_number(1488),
	g_nThreads(0),
	g_hShutdownEvent(nullptr),
	g_hIOCompletionPort(nullptr),
	g_hAcceptEvent(nullptr),
	acceptThread(),
	listenSocket(0)

{
}

SccServerNPIO::~SccServerNPIO()
{
}
