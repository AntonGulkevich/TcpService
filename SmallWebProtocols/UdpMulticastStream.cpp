#include "UdpMulticastStream.h"

UdpMulticastStream::UdpMulticastStream(SOCKET socketId_,const SOCKADDR_IN & addrinfo) 
: INetStream(socketId_, addrinfo)
{
}

INT UdpMulticastStream::Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	if (_socketId > 0) {
		auto sendedBytes = sendto(_socketId, pBuff, buffSize, NULL, PSOCKADDR(&_destinationAddres), sizeof(_destinationAddres));
		if (sendedBytes == SOCKET_ERROR)
			sendedBytes = -WSAGetLastError();
		return sendedBytes;
	}
	return SOCKET_ERROR;
}

INT UdpMulticastStream::Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	if (_socketId > 0) {
		if (WaitForReadEvent(milliseconds) == true)
		{
			INT addrlen = sizeof(_destinationAddres);
			auto recBytes = recvfrom(_socketId, pBuff, buffSize, NULL, PSOCKADDR(&_destinationAddres), &addrlen);
			if (recBytes >= 0)
				return recBytes;
			return -WSAGetLastError();
		}
		// If timed out
		return -WSAETIMEDOUT;
	}
	return SOCKET_ERROR;

}


UdpMulticastStream::~UdpMulticastStream()
{
	if (_socketId)
		closesocket((_socketId));
}

LPUdpMulticastStream UdpConnector::InitHost(LPCTSTR hostName, USHORT port, PINT errCode)
{
	auto err = 0; 
	if (errCode == nullptr)
		errCode = &err;
	*errCode = err;
	auto socketDecr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketDecr == INVALID_SOCKET)
	{
		err = -WSAGetLastError();
		*errCode = err;
		return nullptr;
	}

	SOCKADDR_IN sockaddr;
	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	if (TcpConnector::resolveHostName(hostName, &(sockaddr.sin_addr)) != 0)
		InetPton(AF_INET, hostName, &(sockaddr.sin_addr));
	sockaddr.sin_port = htons(port);

	return new UdpMulticastStream(socketDecr, sockaddr);
}

LPUdpMulticastStream UdpConnector::ConnectToMulticastGroup(LPCTSTR hostName, USHORT port, PINT errCode)
{

	auto err = 0;
	if (errCode == nullptr)
		errCode = &err;
	*errCode = err;
	auto socketDecr = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketDecr == INVALID_SOCKET) {
		err = -WSAGetLastError();
		*errCode = err;
		return nullptr;
	}

	auto iRes = 1;
	// Share port
	if (setsockopt(socketDecr, SOL_SOCKET, SO_REUSEADDR,reinterpret_cast<CHAR*>(&iRes), sizeof(iRes)) < 0) {
		closesocket(socketDecr);
		err = -WSAGetLastError();
		*errCode = err;
		return nullptr;
	}

	SOCKADDR_IN sockaddr;
	ZeroMemory(&sockaddr, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socketDecr, reinterpret_cast<PSOCKADDR>(&sockaddr), sizeof(sockaddr)) < 0)
	{
		err = -WSAGetLastError();
		*errCode = err;
		closesocket(socketDecr);
		return nullptr;
	}

	IP_MREQ mreq;
	ZeroMemory(&mreq, sizeof(mreq));
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (TcpConnector::resolveHostName(hostName, &(mreq.imr_multiaddr)) != 0)
		InetPton(AF_INET, hostName, &(mreq.imr_multiaddr));

	/* use setsockopt() to request that the kernel join a multicast group */
	if (setsockopt(socketDecr, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<CHAR*>(&mreq), sizeof(mreq)) < 0) {
		err = -WSAGetLastError();
		*errCode = err;
		closesocket(socketDecr);
		return nullptr;
	}

	return new UdpMulticastStream(socketDecr, sockaddr);
}