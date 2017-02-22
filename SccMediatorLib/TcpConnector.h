#pragma once
#include "TcpIOStream.h"

class  TcpConnector
{
public:
	static TcpIOStream* connect(LPCTSTR hostname, USHORT port);
private:
	static INT resolveHostName(LPCTSTR hostname, PIN_ADDR addr);
};
