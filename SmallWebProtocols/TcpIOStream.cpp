#include "TcpIOStream.h"
#include <iostream>

TcpIOStream::TcpIOStream(SOCKET sDescr, const SOCKADDR_IN & addres):INetStream(sDescr, addres) 
{
	//TCHAR ip[INET_ADDRSTRLEN];
	//auto ipAddress = InetNtop(addres->sin_family, PVOID(&addres->sin_addr), ip, INET_ADDRSTRLEN);
}


TcpIOStream::~TcpIOStream()
{
	if (_socketId != INVALID_SOCKET)
		closesocket(_socketId);
}

INT TcpIOStream::Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	std::lock_guard<std::mutex> lock(_ioMutex);
	if (_socketId > 0) {
		// Wait for ready read event
		if (WaitForReadEvent(milliseconds))
		{
			auto recBytes = recv(_socketId, pBuff, buffSize, 0);
			if (recBytes >= 0)
				return recBytes;
			return -WSAGetLastError();
		}
		// If timed out
		return -WSAETIMEDOUT;
	}
	return SOCKET_ERROR;
}

INT TcpIOStream::Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	std::lock_guard<std::mutex> lock(_ioMutex);
	if (_socketId > 0) {
		// Wait for ready write event
		if (IsReadyToWrite(milliseconds))
		{
			auto sendBytes = send(_socketId, pBuff, buffSize, 0);
			if (sendBytes > 0)
				return sendBytes;
			return -WSAGetLastError();
		}
		// If timed out
		return -WSAETIMEDOUT;
	}
	return SOCKET_ERROR;
}
