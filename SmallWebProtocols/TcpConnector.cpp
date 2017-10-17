#include "TcpConnector.h"

std::unique_ptr<TcpIOStream> TcpConnector::connect(LPCTSTR hostname, USHORT port, PINT errCode)
{
	SOCKADDR_IN address;
	ZeroMemory(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if (resolveHostName(hostname, &(address.sin_addr)) != 0)
		InetPton(AF_INET, hostname, &(address.sin_addr));

	auto err = 0;
	if (errCode == nullptr)
		errCode = &err;
	*errCode = err;

	auto socketDecr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketDecr == INVALID_SOCKET)
	{
		err = -WSAGetLastError();
		*errCode = err;
		return nullptr;
	}

	if (::connect(socketDecr, reinterpret_cast<PSOCKADDR>(&address), sizeof(address)))
	{
		err = -WSAGetLastError();
		*errCode = err;
		closesocket(socketDecr);
		return nullptr;
	}
	std::unique_ptr<TcpIOStream> uniqStream(new TcpIOStream(socketDecr, address));
	return uniqStream;
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
