#pragma once
#include "TcpConnector.h"

#define DEFAULT_UDP_ADDRESS_A	("239.2.2.8")
#define DEFAULT_UDP_ADDRESS_W	(_T("239.2.2.8"))
#define DEFAULT_PORT			(1488)

#ifdef UNICODE 
#define DEFAULT_UDP_ADDRESS DEFAULT_UDP_ADDRESS_W
#else
#define DEFAULT_UDP_ADDRESS DEFAULT_UDP_ADDRESS_A
#endif
#include "INetStream.h"

class UdpConnector;

typedef class UdpMulticastStream : public INetStream
{
	friend UdpConnector;

	explicit UdpMulticastStream(SOCKET socketId_, const SOCKADDR_IN & addrinfo);
	UdpMulticastStream() = delete;

public:
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero or positive(count of bytes sended)
	INT Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) override;
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero on disconnection or positive(count of bytes sended) on successfully sent
	INT Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE) override;
	virtual ~UdpMulticastStream();
} *LPUdpMulticastStream;

class UdpConnector
{
public:
	static LPUdpMulticastStream InitHost(LPCTSTR hostName, USHORT port, PINT errCode = nullptr);
	static LPUdpMulticastStream ConnectToMulticastGroup(LPCTSTR hostName, USHORT port, PINT errCode = nullptr);
};

