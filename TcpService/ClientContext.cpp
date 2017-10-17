#include "ClientContext.h"
#include <iostream>

ClientContext::ClientContext(SOCKET socket) : _totalBytesCount(0), _sentBytesCount(0), _socket(socket), _ioCode(0), _ipAddressStr(nullptr), _portNumber(0), _clientID(0)
{
	// init buffers
	_overlapped = new OVERLAPPED;
	_wsaBuf = new WSABUF;
	// zero memory
	ZeroMemory(_overlapped, sizeof(OVERLAPPED));
	ZeroMemory(_buffer, MAX_BUFFER_LEN);
	// set wsabuf
	_wsaBuf->buf = _buffer;
	_wsaBuf->len = MAX_BUFFER_LEN;
	// set initial values
	_ioCode = NULL;
	_totalBytesCount = NULL;
	_sentBytesCount = NULL;
}

void ClientContext::SetIpAddress(PCHAR ipAddressPtr)
{
	if (_ipAddressStr == nullptr)
		_ipAddressStr = new CHAR[INET_ADDRSTRLEN];
	memcpy_s(_ipAddressStr, INET_ADDRSTRLEN, ipAddressPtr, INET_ADDRSTRLEN);
}

void ClientContext::AssignClientID(const PSOCKADDR_IN info)
{
	CHAR ip_char[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &info->sin_addr, ip_char, sizeof(ip_char));
	SetIpAddress(ip_char);
	_portNumber = htons(info->sin_port);
	// calculating ID base on XOR hash ip and hash port 
	_clientID = std::hash_value(ip_char) ^ std::hash_value(GetPort());
}

void ClientContext::ResetWSABUF()
{
	ZeroMemory(_buffer, MAX_BUFFER_LEN);
	_wsaBuf->buf = _buffer;
	_wsaBuf->len = MAX_BUFFER_LEN;
}

ClientContext::~ClientContext()
{
	if (_socket) {
		closesocket(_socket);
		_socket = NULL;
	}

	delete[] _ipAddressStr;
	delete _overlapped;
	delete _wsaBuf;
}
