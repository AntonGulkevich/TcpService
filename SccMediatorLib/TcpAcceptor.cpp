#include "TcpAcceptor.h"

TcpAcceptor::TcpAcceptor(LPCTSTR hostName, USHORT portNumber, int backlog = SOMAXCONN) : sockDescr(0),
                                                                                         us_pornNumber(portNumber),
                                                                                         strHostName(hostName),
                                                                                         isListening(false),
                                                                                         backlog(backlog)
{
}

TcpAcceptor::~TcpAcceptor()
{
	if (sockDescr != INVALID_SOCKET)
		closesocket(sockDescr);
}

int TcpAcceptor::Start()
{
	if (isListening) return SOCKET_ERROR;
	sockDescr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockDescr == INVALID_SOCKET) return INVALID_SOCKET;

	SOCKADDR_IN address;
	ZeroMemory(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(us_pornNumber);
	strHostName ? InetPton(AF_INET, strHostName, &(address.sin_addr)) : address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(sockDescr, PSOCKADDR(&address), sizeof(address)) == SOCKET_ERROR) return SOCKET_ERROR;

	if (listen(sockDescr, backlog) == SOCKET_ERROR) return SOCKET_ERROR;

	isListening = true;

	return NO_ERROR;
}

TcpIOStream* TcpAcceptor::Accept() const
{
	if (!isListening) return nullptr;

	SOCKADDR_IN address;
	INT addressLen = sizeof(SOCKADDR_IN);
	ZeroMemory(&address, addressLen);

	auto accepted_socket_dsc = accept(sockDescr, PSOCKADDR(&address), &addressLen);

	return accepted_socket_dsc == INVALID_SOCKET ? nullptr : new TcpIOStream(accepted_socket_dsc, &address);
}
