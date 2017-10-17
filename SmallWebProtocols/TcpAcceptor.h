#pragma once
#include "TcpIOStream.h"

class  TcpAcceptor
{	
	int _backlog;
	SOCKET _sockDescr;
	USHORT _usPortNumber;
	LPCTSTR _strHostName;
	bool _isListening;
public:
	TcpAcceptor(LPCTSTR hostName, USHORT portNumber, INT backlog);
	~TcpAcceptor();

	int Start();
	TcpIOStream* Accept() const;
};

typedef TcpAcceptor * PTcpAcceptor;
typedef const TcpAcceptor * PCTcpAcceptor;
