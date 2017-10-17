#pragma once
#include "TcpIOStream.h"

class  TcpConnector
{
public:
	static std::unique_ptr<TcpIOStream> connect(LPCTSTR hostname, USHORT port, PINT errCode = nullptr);
	static INT resolveHostName(LPCTSTR hostname, PIN_ADDR addr);
};
