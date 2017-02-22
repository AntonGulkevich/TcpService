#include "TcpIOStream.h"




TcpIOStream::TcpIOStream(SOCKET sDescr, const PSOCKADDR_IN addres) :socketId(sDescr), portNumber(ntohs(addres->sin_port))
{
	TCHAR ip[INET_ADDRSTRLEN];
	auto ipAddress = InetNtop(addres->sin_family, PVOID(&addres->sin_addr), ip, INET_ADDRSTRLEN);
}


USHORT TcpIOStream::GetPortNumber() const
{
	return portNumber;
}

TcpIOStream::~TcpIOStream()
{
	if (socketId != INVALID_SOCKET) closesocket(socketId);
}

int TcpIOStream::Recieve(PNZCH pBuff, INT buffSize, DWORD milliseconds) const
{
	if (socketId > 0) {

		if (milliseconds == INFINITE) {
			return recv(socketId, pBuff, buffSize, 0);
		}
		if (WaitForReadEvent(milliseconds) == true)
		{
			return recv(socketId, pBuff, buffSize, 0);
		}
		return TIMEDOUT;
	}
	return SOCKET_ERROR;
}

int TcpIOStream::Send(PCNZCH pBuff, INT buffSize, DWORD milliseconds) const
{
	if (socketId > 0) {

		if (milliseconds == INFINITE)
			return send(socketId, pBuff, buffSize, 0);
		if (IsReadyToWrite(milliseconds) == true)
		{
			return send(socketId, pBuff, buffSize, 0);
		}
		return TIMEDOUT;
	}
	return SOCKET_ERROR;
}

bool TcpIOStream::WaitForReadEvent(DWORD timeout) const
{
	fd_set readfds;
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&readfds);
	FD_SET(socketId, &readfds);
	if (select(socketId + 1, &readfds, nullptr, nullptr, &tv) > 0)
		return true;
	return false;
}

bool TcpIOStream::IsReadyToWrite(DWORD timeout) const
{
	fd_set writefds;
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&writefds);
	FD_SET(socketId, &writefds);
	if (select(socketId + 1, nullptr, &writefds, nullptr, &tv) > 0)
		return true;
	return false;
}
