#include "TcpConnector.h"


TcpIOStream* TcpConnector::connect(LPCTSTR hostname, USHORT port)
{
	SOCKADDR_IN address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if (resolveHostName(hostname, &(address.sin_addr)) != 0)
		InetPton(AF_INET, hostname, &(address.sin_addr));

	auto socketDecr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketDecr == INVALID_SOCKET)
		return nullptr;

	if (::connect(socketDecr, reinterpret_cast<PSOCKADDR>(&address), sizeof(address)) == NO_ERROR)
		return new TcpIOStream(socketDecr, &address);

	closesocket(socketDecr);
	return nullptr;
}

INT TcpConnector::resolveHostName(LPCTSTR hostname, PIN_ADDR addr)
{
	PADDRINFOW result = nullptr;

	auto err = GetAddrInfo(hostname, nullptr, nullptr, &result);
	if (err == 0)
	{
		memcpy(addr, &reinterpret_cast<PSOCKADDR_IN>(result->ai_addr)->sin_addr, sizeof(IN_ADDR));
		FreeAddrInfo(result);
	}
	return err;
}
