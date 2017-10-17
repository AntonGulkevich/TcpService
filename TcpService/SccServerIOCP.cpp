#include "SccServerIOCP.h"

LPSccServerIOCP SccServerIOCP::_serverInstPtr;

UINT SccServerIOCP::GetHardwareConcurrency() noexcept
{
	return std::thread::hardware_concurrency() * THREADS_PER_PROCESS;
}

UINT SccServerIOCP::WorkerThread(INT nThreadNo)
{
	LPVOID lpContext = nullptr;
	LPOVERLAPPED lpOverlapped = nullptr;
	PClientContext pClientContext = nullptr;
	DWORD bytesTransfered = 0;
	DWORD bytes = 0;
	DWORD flags = 0;
	INT nBytesSent;
	LPWSABUF p_wbuf = nullptr;
	LPOVERLAPPED p_ol = nullptr;

	while (WaitForSingleObject(_shutdownEvent, NULL) != WAIT_OBJECT_0)
	{
		auto bReturn = GetQueuedCompletionStatus(_iocpMain, &bytesTransfered,
			reinterpret_cast<PULONG_PTR>(&lpContext),
			&lpOverlapped,
			INFINITE);

		if (WaitForSingleObject(_shutdownEvent, NULL) == WAIT_OBJECT_0)
		{
			std::lock_guard<std::mutex> lc(_threadClosing);
			debugCoutSync("Server is stopping...", ++_threadClosingCounter, "/", _workerThreadsCount);
			return 0;
		}

		if (lpContext == nullptr)
		{
			debugCoutSync("Error in WorkerThread: unable to access client info memory address.");
			return ERROR_ACCESS_DENIED; // fatal here
		}

		pClientContext = static_cast<PClientContext>(lpContext);

		if (bReturn == FALSE)
		{
			if (GetLastError() == ERROR_NETNAME_DELETED)
			{
				debugCoutSync("The specified network name is no longer available.");
			}
			else
			{
				debugCoutSync("IOCP error: ", WSAGetLastError(), " Thread id: ", nThreadNo);
			}
			RemoveClient(pClientContext->GetId());
			continue;
		}

		if (bytesTransfered == NULL)
		{
			debugCoutSync("Client Diconnected. Thread id: ", nThreadNo);
			RemoveClient(pClientContext->GetId());
			continue;
		}

		p_wbuf = pClientContext->GetWSABUFPtr();
		p_ol = pClientContext->GetOVERLAPPEDPtr();

		switch (pClientContext->GetIOCode())
		{
		case OP_READ:
		{
			pClientContext->IncrSentBytes(bytesTransfered);
			if (pClientContext->GetSentBytes() < pClientContext->GetTotalBytes())
			{
				pClientContext->SetIOCode(OP_READ);

				p_wbuf->buf += pClientContext->GetSentBytes();
				p_wbuf->len = pClientContext->GetTotalBytes() - pClientContext->GetSentBytes();

				//Overlapped send
				nBytesSent = WSASend(pClientContext->GetSocket(), p_wbuf, 1,
					&bytes, NULL, p_ol, nullptr);

				if ((nBytesSent == SOCKET_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
				{
					debugCoutSync("Connection lost.");
					RemoveClient(pClientContext->GetId());
					break;
				}
			}
			else
			{
				pClientContext->SetIOCode(OP_WRITE);
				pClientContext->ResetWSABUF();
				flags = 0;
				auto nBytesRecv = WSARecv(pClientContext->GetSocket(), p_wbuf, 1,
					&bytes, &flags, p_ol, nullptr);

				auto lastWsaError = WSAGetLastError();

				if (lastWsaError == WSAECONNABORTED)
				{
					debugCoutSync("Client unexpectedly closed the connection.");
					RemoveClient(pClientContext->GetId());
					break;
				}
				if ((nBytesRecv == SOCKET_ERROR) && (lastWsaError != WSA_IO_PENDING))
				{
					AddErrorLogEntry(_T("Client Diconnected: thread Error occurred while executing WSARecv() "), lastWsaError);
					RemoveClient(pClientContext->GetId());
					break;
				}
			}
			break;
		}
		case OP_WRITE:
		{
			// read data
			size_t offset = 0;
			DWORD totalbytes = 0;
			auto sizeRec = Message::ReadPODFromBytes<UINT>(p_wbuf->buf, offset);
			auto commandRec = static_cast<CommandHeader>(Message::ReadPODFromBytes<UINT>(p_wbuf->buf, offset += sizeof(UINT)));
			std::vector<UINT> parametersRec;
			for (offset += sizeof(UINT); offset < sizeRec; offset += sizeof(UINT))
				parametersRec.emplace_back(Message::ReadPODFromBytes<UINT>(p_wbuf->buf, offset));
			// process data
			switch (commandRec)
			{
			case CommandHeader::GetAvalibleChannelsIn:
			{
				//first 4 bits represent 429 channels, 5th bit - 708
				pClientContext->SetBufferT(GetChannelSatesIn(parametersRec.at(0), pClientContext->GetId()));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::GetAvalibleChannelsOut:
			{
				//first 4 bits represent 429 channels, 5th bit - 708
				pClientContext->SetBufferT(GetChannelSatesOut(parametersRec.at(0), pClientContext->GetId()));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ReleaseChannel429In:
			{
				pClientContext->SetBufferT(ReleaseChannelIfAvaliableFor(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In429));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ReleaseChannel429Out:
			{
				pClientContext->SetBufferT(ReleaseChannelIfAvaliableFor(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), Out429));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ReleaseChannel708In:
			{
				pClientContext->SetBufferT(ReleaseChannelIfAvaliableFor(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In708));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ReleaseChannel708Out:
			{
				pClientContext->SetBufferT(ReleaseChannelIfAvaliableFor(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), Out708));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ConnectForced429In:
			{
				pClientContext->SetBufferT(ConnectForced(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In429));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ConnectForced429Out:
			{
				pClientContext->SetBufferT(ConnectForced(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), Out429));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ConnectForced708In:
			{
				pClientContext->SetBufferT(ConnectForced(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In708));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::ConnectForced708Out:
			{
				pClientContext->SetBufferT(ConnectForced(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), Out708));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::Ping:
			{
				pClientContext->SetBufferT(0);
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::Disconnect:
			{
				debugCoutSync("Client request disconnection: successfully disconnected.");
				RemoveClient(pClientContext->GetId());
				pClientContext = nullptr;
				break;
			}
			case CommandHeader::GetDeviceCount:
			{
				pClientContext->SetBufferT(SccLibInterop::GetDeviceCount());
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::Set429InputChannelParams:
				ProcessIncomingCommand<INT, UINT, UINT, Arinc429Rate, Arinc429ParityTypeIn>
					(pClientContext, totalbytes, SccLibInterop::Set429InputChannelParams, parametersRec, true, In429);
				break;
			case CommandHeader::Set429OutputChannelParams:
				ProcessIncomingCommand<INT, UINT, UINT, Arinc429Rate, Arinc429ParityTypeOut>
					(pClientContext, totalbytes, SccLibInterop::Set429OutputChannelParams, parametersRec, true, Out429);
				break;
			case CommandHeader::Get429OutputBufferWordsCount:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Get429OutputBufferWordsCount, parametersRec);
				break;
			case CommandHeader::Get708OutputBufferWordsCount:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Get708OutputBufferWordsCount, parametersRec);
				break;
			case CommandHeader::Get429OutputBufferMicroseconds:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Get429OutputBufferMicroseconds, parametersRec);
				break;
			case CommandHeader::Get708OutputBufferMicroseconds:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Get708OutputBufferMicroseconds, parametersRec);
				break;
			case CommandHeader::Set429InputBufferLength:
				ProcessIncomingCommand<INT, UINT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Set429InputBufferLength, parametersRec, true, In429);
				break;
			case CommandHeader::Set708InputBufferLength:
				ProcessIncomingCommand<INT, UINT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::Set708InputBufferLength, parametersRec, true, In708);
				break;
			case CommandHeader::ResetOut429Channel:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::ResetOut429Channel, parametersRec, true, Out429);
				break;
			case CommandHeader::ResetOut708Channel:
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::ResetOut708Channel, parametersRec, true, Out708);
				break;
			case CommandHeader::ResetIn429Channel: {
				ResetBuffer(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In429);
				ProcessIncomingCommand<INT, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::ResetIn429Channel, parametersRec, true, In429);
				break;
			}
			case CommandHeader::ResetIn708Channel:
				ResetBuffer(parametersRec.at(0), pClientContext->GetId(), parametersRec.at(1), In708);
				ProcessIncomingCommand<int, UINT, UINT>(pClientContext, totalbytes, SccLibInterop::ResetIn708Channel, parametersRec, true, In708);
				break;
			case CommandHeader::GetDeviceNumsRaw:
			{
				auto deviceCount = SccLibInterop::GetDeviceNumsRaw(reinterpret_cast<UINT*>(p_wbuf->buf), parametersRec.at(0));
				// If error occured in GetDeviceNumsRaw
				if (deviceCount < 0)
				{
					pClientContext->SetBufferT(deviceCount);
					totalbytes = sizeof(INT);
				}
				// No error occured and no device connected
				if (deviceCount == 0)
				{
					pClientContext->SetBufferT(NULL);
					totalbytes = sizeof(INT);
				}
				// No error occured and one or more devices are avaliable
				if (deviceCount > 0)
					totalbytes = deviceCount * sizeof(UINT);
				break;
			}
			case CommandHeader::Send429WordsRaw:
			{
				if (IsChannelAvaliableFor(parametersRec.at(0), Out429, parametersRec.at(1), pClientContext->GetId()))
				{
					auto byteOffset = sizeof(UINT) * 5;
					pClientContext->SetBufferT(SccLibInterop::Send429WordsRaw(
						parametersRec.at(0), // Serial number
						parametersRec.at(1), // Channel number
						reinterpret_cast<Word429*>(p_wbuf->buf + byteOffset), // Words429 array
						parametersRec.at(2))); // Arinc429 count);
				}
				else
					pClientContext->SetBufferT(ACCESS_DENIED);
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::Receive429WordsRaw:
			{
				_devicesMapMutex.lock();
				auto resDevF = _devicesMap.find(parametersRec.at(NULL));
				// If no device with this SN
				if (resDevF == _devicesMap.end())
				{
					pClientContext->SetBufferT(DEVICE_NOT_AVAILABLE);
					totalbytes = sizeof(INT);
				}
				else {
					// If  device exists
					auto realSize = resDevF->second->Receive429WordsRaw(pClientContext->GetId(), // owners ID
						parametersRec.at(1), // Channel number
						reinterpret_cast<Word429*>(p_wbuf->buf + sizeof(UINT)), // Words429 buffer
						parametersRec.at(2) > MAX_WORD429_COUNT ? MAX_WORD429_COUNT : parametersRec.at(2));// Arinc429 words count
					pClientContext->SetBufferT(realSize);
					totalbytes = realSize > 0 ? realSize * sizeof(Word429) + sizeof(UINT) : sizeof(UINT);
				}
				_devicesMapMutex.unlock();
				break;
			}
			case CommandHeader::Send708WordsRaw:
			{
				if (IsChannelAvaliableFor(parametersRec.at(0), Out708, parametersRec.at(1), pClientContext->GetId()))
				{
					auto byteOffset = sizeof(UINT) * 5;
					pClientContext->SetBufferT(SccLibInterop::Send708WordsRaw(
						parametersRec.at(0), // Serial number
						parametersRec.at(1), // Channel number
						reinterpret_cast<Word708*>(p_wbuf->buf + byteOffset), // Words708 array
						parametersRec.at(2))); // Arinc708 count
				}
				else
					pClientContext->SetBufferT(ACCESS_DENIED);
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::GetPinConfiguration:
			{
				pClientContext->SetBufferT(SccLibInterop::GetPinConfiguration(parametersRec.at(NULL)));
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::GetDllVersion:
			{
				pClientContext->SetBufferT(SccLibInterop::GetDllVersion());
				totalbytes = sizeof(UINT);
				break;
			}
			case CommandHeader::Receive708WordsRaw:
			{
				_devicesMapMutex.lock();
				auto resDevF = _devicesMap.find(parametersRec.at(NULL));

				// If no device with this SN
				if (resDevF == _devicesMap.end())
				{
					pClientContext->SetBufferT(DEVICE_NOT_AVAILABLE);
					totalbytes = sizeof(UINT);
					_devicesMapMutex.unlock();
					break;
				}

				// If  device exists
				auto realSize = resDevF->second->Receive708WordsRaw(pClientContext->GetId(), // owners ID
					parametersRec.at(1), // Channel number
					reinterpret_cast<Word708*>(p_wbuf->buf + sizeof(UINT)), // Words708 array
					parametersRec.at(2) > MAX_WORD708_COUNT ? MAX_WORD708_COUNT : parametersRec.at(2)); // Arinc708 count
				_devicesMapMutex.unlock();
				pClientContext->SetBufferT(realSize);
				totalbytes = realSize > 0 ? realSize * sizeof(Word708) + sizeof(UINT) : sizeof(UINT);
				break;
			}
			default:
			{
				//todo: fatal here.
				AddErrorLogEntry(_T("Wrong command."), INVALID_COMMAND_DATA);
				break;
			}
			}
			if (pClientContext != nullptr)
			{
				pClientContext->SetIOCode(OP_READ);
				pClientContext->SetTotalBytes(totalbytes);
				pClientContext->SetSentBytes(NULL);
				p_wbuf->len = totalbytes;
				nBytesSent = WSASend(pClientContext->GetSocket(), p_wbuf, 1,
					&bytes, NULL, p_ol, nullptr);

				if ((SOCKET_ERROR == nBytesSent) && (WSA_IO_PENDING != WSAGetLastError()))
				{
					AddEventLogEntry(_T("Client Diconnected. Error occurred while executing WSASend()."), EVENTLOG_ERROR_TYPE);
					RemoveClient(pClientContext->GetId());
				}
			}
			break;
		}
		default:
		{
			// Almost impossible to come here
			//	todo: fatal here 
			AddErrorLogEntry(_T("Wrong IO code."), INVALID_COMMAND_DATA);
			break;
		}
		}
	}
	return NO_ERROR;
}

void SccServerIOCP::AcceptThread()
{
	WSANETWORKEVENTS WSAEvents;
	while (WaitForSingleObject(_shutdownEvent, NULL) != WAIT_OBJECT_0)
	{
		if (WSAWaitForMultipleEvents(1, &_wsaAcceptEvent, FALSE, ACCEPT_WAIT_TIMEOUT, FALSE) != WSA_WAIT_TIMEOUT)
		{
			WSAEnumNetworkEvents(_listenSocket, _wsaAcceptEvent, &WSAEvents);
			if ((WSAEvents.lNetworkEvents & FD_ACCEPT) && (WSAEvents.iErrorCode[FD_ACCEPT_BIT] == NULL))
				AcceptConnection();
		}
	}
}

void SccServerIOCP::AcceptConnection()
{
	sockaddr_in ClientAddress;
	int nClientLength = sizeof(ClientAddress);

	//Accept remote connection attempt from the client
	auto acceptedSocket = accept(_listenSocket, reinterpret_cast<PSOCKADDR>(&ClientAddress), &nClientLength);

	if (acceptedSocket == INVALID_SOCKET)
	{
		AddErrorLogEntry(_T("accept in AcceptConnection"), WSAGetLastError());
		return;
	}

	auto context = std::make_unique<ClientContext>(acceptedSocket);
	context->SetIOCode(OP_READ);
	context->AssignClientID(&ClientAddress);

	debugCoutSync("Connected. IP: ", context->GetIpAddress(), ". Port: ", context->GetPort(), ". ID: ", context->GetId());

	//Associate the socket with IOCP
	if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(context->GetSocket()),
		_iocpMain,
		reinterpret_cast<ULONG_PTR>(context.get()),
		IGNORE) == nullptr)
	{
		AddErrorLogEntry(L"CreateIoCompletionPort in AcceptConnection", WSAGetLastError());
		RemoveClient(context->GetId());
		return;
	}

	context->SetIOCode(OP_WRITE);

	auto p_wbuf = context->GetWSABUFPtr();
	auto p_ol = context->GetOVERLAPPEDPtr();

	//Update data.
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;

	auto nBytesRecv = WSARecv(context->GetSocket(), p_wbuf, 1,
		&dwBytes, &dwFlags, p_ol, nullptr);

	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		AddErrorLogEntry(_T("WSARecv in AcceptConnection"), WSAGetLastError());
		return;
	}

	if (!AddToClientList(std::move(context)))
	{
		AddEventLogEntry(_T("Unable to add client with same ID. IP and port address. "), EVENTLOG_ERROR_TYPE);
		return;
	}
}

void SccServerIOCP::OnDeviceAttached(DWORD deviceSerialNumber)
{
	_serverInstPtr->AddDevice(deviceSerialNumber);
}

void SccServerIOCP::OnDeviceDetached(DWORD deviceSerialNumber)
{
	_serverInstPtr->RemoveDevice(deviceSerialNumber);
}

INT SccServerIOCP::InitializeDeviceNotificationThread()
{
	auto err = 0;
	// Initalization UDP multicast members
	_UDPstream = UdpConnector::InitHost(_strUDPHostName, _usUDPportNumber, &err);
	if (_UDPstream == nullptr)
	{
		AddErrorLogEntry(_T("InitHost in InitializeDeviceNotificationThread "), err);
		return err;
	}
	return err;
}

INT SccServerIOCP::InitializeWSASocket()
{
	auto err = 0;
	//Overlapped I/O follows the model established in Windows and can be performed only on 
	//sockets created through the WSASocket function 
	_listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (_listenSocket == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("WSASocket in InitializeWSASocket "), err);
		return -err;
	}
	ZeroMemory(&_serverAddress, sizeof(SOCKADDR_IN));
	//Fill up the address structure
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_addr.s_addr = INADDR_ANY;
	_serverAddress.sin_port = htons(_usTCPportNumber);

	if (bind(_listenSocket, reinterpret_cast<PSOCKADDR>(&_serverAddress), sizeof(_serverAddress)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("bind in InitializeWSASocket "), err);
		return -err;
	}

	if (listen(_listenSocket, SOMAXCONN))
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("listen in InitializeWSASocket "), err);
		return -err;
	}

	_wsaAcceptEvent = WSACreateEvent();
	if (_wsaAcceptEvent == WSA_INVALID_EVENT)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("WSACreateEvent in InitializeWSASocket "), err);
		return -err;
	}

	if (WSAEventSelect(_listenSocket, _wsaAcceptEvent, FD_ACCEPT) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("WSAEventSelect in InitializeWSASocket "), err);
		return -err;
	}
	return err;
}

INT SccServerIOCP::AddDevice(const DWORD deviceSerialNumber)
{
	debugCoutSync("Added device. SN: ", deviceSerialNumber);

	_devicesMapMutex.lock();
	_devicesMap.emplace(std::make_pair(deviceSerialNumber, std::make_unique<VirtualDevice>(deviceSerialNumber)));
	_devicesMapMutex.unlock();

	Message message(CommandHeader::Attached);
	message.Append(deviceSerialNumber);
	message.Append(PCHAR(_hostIP), size_t(INET_ADDRSTRLEN));

	if (_UDPstream != nullptr)
		return _UDPstream->Send(message.GetRawData(), message.GetBytesCount());
	return UDP_HOST_OFFLINE;
}

INT SccServerIOCP::RemoveDevice(DWORD deviceSerialNumber)
{
	debugCoutSync("Device removed. SN: ", deviceSerialNumber);

	_devicesMapMutex.lock();
	_devicesMap.erase(deviceSerialNumber);
	_devicesMapMutex.unlock();

	Message message(CommandHeader::Detached);
	message.Append(deviceSerialNumber);
	message.Append(PCHAR(_hostIP), size_t(INET_ADDRSTRLEN));

	if (_UDPstream != nullptr)
		return _UDPstream->Send(message.GetRawData(), message.GetBytesCount());
	return UDP_HOST_OFFLINE;
}

BOOL SccServerIOCP::IsChannelAvaliableFor(DWORD deviceSerialNumber, ChannelTypeFull type, UINT channelNumber, size_t ownerId)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto fRes = _devicesMap.find(deviceSerialNumber);
	if (fRes == _devicesMap.end())
		return DEVICE_NOT_AVAILABLE;
	return fRes->second->IsAvaliableFor(type, channelNumber, ownerId);
}

INT SccServerIOCP::GetChannelSatesIn(DWORD deviceSerialNumber, size_t ownerID)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto currentDevice = _devicesMap.find(deviceSerialNumber);
	return currentDevice != _devicesMap.end() ? currentDevice->second->GetChannelsStateIn(ownerID) : DEVICE_NOT_AVAILABLE;
}

INT SccServerIOCP::GetChannelSatesOut(DWORD deviceSerialNumber, size_t ownerID)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto currentDevice = _devicesMap.find(deviceSerialNumber);
	return currentDevice != _devicesMap.end() ? currentDevice->second->GetChannelsStateOut(ownerID) : DEVICE_NOT_AVAILABLE;
}

INT SccServerIOCP::InitializeIOCP()
{
	_iocpMain = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, NULL, NULL);
	if (_iocpMain == nullptr)
		return SccGetLastError;
	return NOERROR;
}

INT SccServerIOCP::InitializeEvents()
{
	auto err = 0;
	_shutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (_shutdownEvent == nullptr)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("CreateEvent failed in InitializeEvents "), err);
		return -err;
	}
	return err;
}

INT SccServerIOCP::InitHostAddress()
{
	auto err = 0;
	char ac[NI_MAXHOST];
	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("gethostname "), err);
		return -err;
	}
	ADDRINFO hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ADDRINFO* result = nullptr;
	if (getaddrinfo(ac, nullptr, &hints, &result) != 0)
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("getaddrinfo "), err);
		return -err;
	}
	auto sockaddr_ip = static_cast<LPSOCKADDR>(result->ai_addr);

	_hostIP = new CHAR[INET_ADDRSTRLEN];
	if (getnameinfo(sockaddr_ip, sizeof(SOCKADDR), _hostIP, INET_ADDRSTRLEN, nullptr, NULL, NI_NUMERICHOST | NI_NUMERICSERV))
	{
		err = WSAGetLastError();
		AddErrorLogEntry(_T("getnameinfo "), err);
		return -err;
	}
	return NOERROR;
}

INT SccServerIOCP::InitializeVirtualDevices()
{
	auto deviceCount = SccLibInterop::GetDeviceCount();
	if (deviceCount > 0)
	{
		auto deviceArray = new UINT[deviceCount];
		auto realCount = SccLibInterop::GetDeviceNumsRaw(deviceArray, deviceCount);
		for (size_t i = 0; i < realCount; ++i) {
			auto err = AddDevice(deviceArray[i]);
			if (err < 0)
			{
				delete[]deviceArray;
				return err;
			}
		}
		delete[]deviceArray;
	}
	return deviceCount;
}

INT SccServerIOCP::InitializeEnvironment()
{
	auto err = 0;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return SccGetLastError;
	}
	_workerThreadsCount = GetHardwareConcurrency();

	err = InitializeIOCP();
	if (err)
		return err;

	err = InitHostAddress();
	if (err)
		return err;

	err = InitializeEvents();
	if (err)
		return err;

	err = InitializeDeviceNotificationThread();
	if (err)
		return err;

	err = InitializeWSASocket();
	if (err)
		return err;

	return  err;
}

void SccServerIOCP::DeInitialize()
{
	if (_listenSocket) {
		closesocket(_listenSocket);
		_listenSocket = NULL;
	}
	if (_iocpMain) {
		CloseHandle(_iocpMain);
		_iocpMain = nullptr;
	}
	if (_shutdownEvent) {
		CloseHandle(_shutdownEvent);
		_shutdownEvent = nullptr;
	}
	WSACleanup();
}

void SccServerIOCP::AddToLog(LPCTSTR message)
{
	// Logging stub
}

void SccServerIOCP::AddErrorLogEntry(PTSTR pszFunction, INT dwError)
{
	WCHAR szMessage[1024];
	StringCchPrintf(szMessage, ARRAYSIZE(szMessage),
		L"Function %s failed with error %d.", pszFunction, dwError);
	AddEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}
/*type:
 *EVENTLOG_SUCCESS
 *EVENTLOG_AUDIT_FAILURE
 *EVENTLOG_AUDIT_SUCCESS
 *EVENTLOG_ERROR_TYPE
 *EVENTLOG_INFORMATION_TYPE
 *EVENTLOG_WARNING_TYPE
 */
void SccServerIOCP::AddEventLogEntry(LPCTSTR message, WORD type)
{
	// Print message to console if _DEBUG is defined else do nothing
	debugWCoutSync(message);
#ifndef _DEBUG
	HANDLE hEventLog = nullptr;
	LPCWSTR lpcwString[2] = { nullptr, nullptr };
	hEventLog = RegisterEventSource(nullptr, _T("SccServerIOCP"));
	if (hEventLog)
	{
		lpcwString[0] = _T("SccServerIOCP");
		lpcwString[1] = message;

		ReportEvent(
			hEventLog, //A handle to the event log. 
			type,// The type of event to be logged				
			0, // Event category
			0, // Event identifier
			nullptr, //A pointer to the current user's security identifier. 
			2, // The number of insert strings in the array pointed to by the lpStrings parameter.
			0, // No binary data
			lpcwString,
			nullptr //A pointer to the buffer containing the binary data.
		);
		DeregisterEventSource(hEventLog);
	}
#endif
}

BOOL SccServerIOCP::ResetBuffer(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, ChannelTypeFull type)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto currentDevice = _devicesMap.find(deviceSerialNumber);
	if (currentDevice == _devicesMap.end())
		return DEVICE_NOT_AVAILABLE;
	auto avRes = currentDevice->second->IsAvaliableFor(type, channelNumber, ownerID);
	if (avRes == TRUE)
		currentDevice->second->ResetBuffer(type, channelNumber);
	return  ACCESS_DENIED;
}

bool SccServerIOCP::AddToClientList(std::unique_ptr<ClientContext> ClientContext)
{
	std::lock_guard<std::mutex> lock(_clientMutex);
	auto ret = _clientsMap.emplace(std::make_pair(ClientContext->GetId(), std::move(ClientContext)));
	if (!ret.second)
	{
		AddEventLogEntry(_T("AddToClientList failed. Client with same id already exists."), EVENTLOG_ERROR_TYPE);
		return false;
	}
	return true;
}

UINT SccServerIOCP::RemoveClient(size_t clientID)
{
	std::lock_guard<std::mutex> lock(_clientMutex);
	std::lock_guard<std::mutex> lockD(_devicesMapMutex);
	UINT count = 0;
	for (auto &&devicePair : _devicesMap)
		count += devicePair.second->RemoveClient(clientID);
	_clientsMap.erase(clientID);
	debugCoutSync("Client ", clientID, " removed. Channel(s) released: ", count, ".");
	return count;
}

void SccServerIOCP::CleanClientList()
{
	std::lock_guard<std::mutex> lock(_clientMutex);
	_clientsMap.clear();
}

bool SccServerIOCP::Stop()
{
	SetEvent(_shutdownEvent);
	if (_acceptThread.joinable())
		_acceptThread.join();

	for (auto i = 0; i < _workerThreadsCount; ++i)
		PostQueuedCompletionStatus(_iocpMain, NULL, NULL, nullptr);

	for (auto it = _workerThreadsVector.begin(); it != _workerThreadsVector.end(); ++it)
	{
		if (it->joinable())
			it->join();
	}

	WSACloseEvent(_wsaAcceptEvent);
	CleanClientList();
	DeInitialize();
	return true;
}

INT SccServerIOCP::Start(LPCTSTR udpHost, USHORT port)
{
	if (udpHost == nullptr)
		_strUDPHostName = DEFAULT_UDP_ADDRESS;
	_usTCPportNumber = port;
	_usUDPportNumber = port + 1;

	// Initialization of Scc Device Lib
	auto err = SccLibInterop::Initialize();
	if (err)
		return err;
	// InitializeEnvironment 
	err = InitializeEnvironment();
	if (err)
	{
		DeInitialize();
		return err;
	}

	_acceptThread = std::thread(&SccServerIOCP::AcceptThread, this);

	for (auto i = 0; i < _workerThreadsCount;)
		_workerThreadsVector.emplace_back(std::thread(&SccServerIOCP::WorkerThread, this, ++i));

	SccLibInterop::SetDeviceAttachedHandler(OnDeviceAttached);
	SccLibInterop::SetDeviceDetachedHandler(OnDeviceDetached);

	InitializeVirtualDevices();

	return NOERROR;
}

SccServerIOCP::SccServerIOCP() :
	_usTCPportNumber(DEFAULT_PORT),
	_usUDPportNumber(DEFAULT_PORT + 1),
	_strUDPHostName(nullptr),
	_hostIP(nullptr),
	_acceptThread(),
	_listenSocket(0),
	_workerThreadsCount(0),
	_shutdownEvent(nullptr),
	_iocpMain(nullptr),
	_wsaAcceptEvent(nullptr),
	_UDPstream(nullptr),
	_threadClosingCounter(0)
{
	_serverInstPtr = this;
	ZeroMemory(&_serverAddress, sizeof(SOCKADDR_IN));
}
SccServerIOCP::~SccServerIOCP()
{
}
