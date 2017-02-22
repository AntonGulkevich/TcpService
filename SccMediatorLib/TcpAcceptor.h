#pragma once
#include "TcpIOStream.h"

class  TcpAcceptor
{
	SOCKET sockDescr;
	USHORT us_pornNumber;
	LPCTSTR strHostName;
	//string m_address;
	bool isListening;
	int backlog;
public:
	TcpAcceptor(LPCTSTR hostName, USHORT portNumber, int backlog);
	~TcpAcceptor();

	int Start();
	TcpIOStream* Accept() const;
};

typedef TcpAcceptor * PTcpAcceptor;
typedef const TcpAcceptor * PCTcpAcceptor;
