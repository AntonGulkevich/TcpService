#include "INetStream.h"


bool INetStream::WaitForReadEvent(DWORD timeout) const
{
	if (timeout == INFINITE)
		return true;
	fd_set readfds;
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&readfds);
	FD_SET(_socketId, &readfds);
	if (select(_socketId + 1, &readfds, nullptr, nullptr, &tv) > 0)
		return true;
	return false;
}

bool INetStream::IsReadyToWrite(DWORD timeout) const
{
	if (timeout == INFINITE)
		return true;
	fd_set writefds;
	struct timeval tv;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&writefds);
	FD_SET(_socketId, &writefds);
	if (select(_socketId + 1, nullptr, &writefds, nullptr, &tv) > 0)
		return true;
	return false;
}

INetStream::~INetStream()
{
}

INetStream::INetStream(SOCKET sDescr, const SOCKADDR_IN & addrinfo) : _destinationAddres(addrinfo), _socketId(sDescr)
{
}
