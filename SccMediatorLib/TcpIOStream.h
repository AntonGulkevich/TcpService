#pragma once

#include <WinSock2.h>
#include <tchar.h>
#include <Ws2tcpip.h>

#define TIMEDOUT (-2)

class TcpIOStream
{
	SOCKET socketId;
	USHORT portNumber;
	//PCTSTR ipAddress;
	// private ctor
	explicit TcpIOStream(SOCKET sDescr, const PSOCKADDR_IN addres);
	bool WaitForReadEvent(DWORD timeout) const;
	bool IsReadyToWrite(DWORD timeout) const;

public:
	friend class TcpConnector;
	friend class TcpAcceptor;

	//PCTSTR GetIpAddress() const;
	USHORT GetPortNumber() const;
	~TcpIOStream();
	int Recieve(PNZCH pBuff, INT buffSize, DWORD milliseconds = INFINITE) const;
	int Send(PCNZCH pBuff, INT buffSize, DWORD milliseconds = INFINITE) const;
};
typedef TcpIOStream * PTcpIOStream;
typedef const TcpIOStream * PCTcpIOStream;
