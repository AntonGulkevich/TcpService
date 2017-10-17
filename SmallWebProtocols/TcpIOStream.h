#pragma once

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <tchar.h>
#include <mutex>
#include "INetStream.h"

class TcpIOStream : public INetStream
{
private:
	explicit TcpIOStream(SOCKET sDescr, const SOCKADDR_IN & addres);
	friend class TcpConnector;
	TcpIOStream() = delete;

public:
	~TcpIOStream();
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is count of bytes received
	// If the connection has been gracefully closed, the return value is zero.
	INT Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) override;
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero or positive(count of bytes sended)
	INT Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) override;
};
typedef TcpIOStream * LPTCPIOSTREAM;
typedef const TcpIOStream * LPCTCPIOSTREAM;
