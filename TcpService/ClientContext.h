#pragma once

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <unordered_map>

#include "../SmallWebProtocols/Message.h"

typedef class ClientContext
{

	LPOVERLAPPED _overlapped;
	LPWSABUF _wsaBuf;

	UINT _totalBytesCount;
	UINT _sentBytesCount;

	SOCKET _socket;
	INT _ioCode;
	CHAR _buffer[MAX_BUFFER_LEN];

	/*Client info*/
	PCHAR _ipAddressStr;
	USHORT _portNumber;

	size_t _clientID;

	VOID SetIpAddress(PCHAR ipAddressPtr);

public:
	// Constructor
	explicit ClientContext(SOCKET socket);

	~ClientContext();

	// Return calculated hash
	auto GetId() const { return _clientID; }

	// Set client port, ip and id
	VOID AssignClientID(const PSOCKADDR_IN info);

	auto GetPort() const { return _portNumber; }

	auto GetIpAddress() const { return _ipAddressStr; }

	VOID SetIOCode(INT n) { _ioCode = n; }

	auto GetIOCode() const { return _ioCode; }

	VOID SetTotalBytes(INT n) { _totalBytesCount = n; }

	auto GetTotalBytes() const { return _totalBytesCount; }

	VOID SetSentBytes(INT n) { _sentBytesCount = n; }

	VOID IncrSentBytes(INT n) { _sentBytesCount += n; }

	auto GetSentBytes() const { return _sentBytesCount; }

	auto GetSocket() const { return _socket; }

	// Write POD into the buffer
	template <typename T>
	auto SetBufferT(T data)
	{
		return memcpy_s(_buffer, MAX_BUFFER_LEN, static_cast<PVOID>(&data), sizeof(T));
	}

	auto GetWSABUFPtr() const { return _wsaBuf; }

	auto GetOVERLAPPEDPtr() const { return _overlapped; }

	VOID ResetWSABUF();
} *PClientContext;