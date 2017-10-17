#include "TcpAcceptor.h"

TcpAcceptor::TcpAcceptor(LPCTSTR hostName, USHORT portNumber, INT backlog = SOMAXCONN) : _backlog(backlog),
                                                                                         _sockDescr(0),
                                                                                         _usPortNumber(portNumber),
                                                                                         _strHostName(hostName),
                                                                                         _isListening(false)
{
}

TcpAcceptor::~TcpAcceptor()
{
	if (_sockDescr != INVALID_SOCKET)
		closesocket(_sockDescr);
}

int TcpAcceptor::Start()
{
	if (_isListening) return SOCKET_ERROR;
	_sockDescr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sockDescr == INVALID_SOCKET) return _sockDescr;

	SOCKADDR_IN address;
	ZeroMemory(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(_usPortNumber);
	_strHostName ? InetPton(AF_INET, _strHostName, &(address.sin_addr)) : address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	if (bind(_sockDescr, PSOCKADDR(&address), sizeof(address)) == SOCKET_ERROR) return SOCKET_ERROR;

	if (listen(_sockDescr, _backlog) == SOCKET_ERROR) return SOCKET_ERROR;

	_isListening = true;

	return NO_ERROR;
}

TcpIOStream* TcpAcceptor::Accept() const
{
	if (!_isListening) return nullptr;

	SOCKADDR_IN address;
	INT addressLen = sizeof(SOCKADDR_IN);
	ZeroMemory(&address, addressLen);

	auto accepted_socket_dsc = accept(_sockDescr, PSOCKADDR(&address), &addressLen);

	return accepted_socket_dsc == INVALID_SOCKET ? nullptr : new TcpIOStream(accepted_socket_dsc, &address);
}
