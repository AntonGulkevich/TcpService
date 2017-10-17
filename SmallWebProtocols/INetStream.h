#pragma once

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <mutex>

class INetStream
{

public:
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero or positive(count of bytes sended)
	virtual INT Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) = 0;
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero on disconnection or positive(count of bytes sended) on successfully sent
	virtual INT Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) = 0;

	INetStream() = delete;
	virtual ~INetStream();
	INetStream(SOCKET sDescr, const SOCKADDR_IN & addrinfo);

protected:
	SOCKADDR_IN _destinationAddres;
	SOCKET _socketId;
	std::mutex _ioMutex;
	bool WaitForReadEvent(DWORD timeout) const;
	bool IsReadyToWrite(DWORD timeout) const;

};

