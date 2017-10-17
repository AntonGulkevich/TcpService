// ServiceManagerWrapper.h

#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <msclr/marshal.h>

#include "../ServiceManager/ServiceConsoleManager.h"

using namespace System;
using namespace System::Configuration;
using namespace System::Runtime::InteropServices;


public ref class ServiceManagerWrapper
{
public:
	static int DeleteSvc(String^ svcName);
	static int InstallSvc(String^ svcName, String^ svcDisplayName, String^ svcDecription, String^ pathToBinary);
	static int StopSvc(String^ svcName, int dwMillisecondsTimeout);
	static int StartSvc(String^ svcName);
	/*
	 * \brief Return current state of the service
	 * \param svcName 
	 * \return return current state of the service or eror code:
	 * 
	 * SERVICE_CONTINUE_PENDING	0x00000005
	 * SERVICE_PAUSE_PENDING	0x00000006
	 * SERVICE_PAUSED			0x00000007
	 * SERVICE_RUNNING			0x00000004
	 * SERVICE_START_PENDING	0x00000002
	 * SERVICE_STOP_PENDING		0x00000003
	 * SERVICE_STOPPED			0x00000001
	 * 
	 * 							0x00000008	The handle does not have access to the service.
	 * 										The handle does not have the SERVICE_QUERY_STATUS access right.
	 * 							0x00000009	The specified handle is invalid.
	 * 							0x0000000A	The specified service name is invalid.
	 *							0x0000000B	The specified service does not exist.
	 *							0x0000000C	The buffer is too small for the SERVICE_STATUS_PROCESS structure. Nothing was written to the structure.
	 *							0x0000000D	The InfoLevel parameter contains an unsupported value.
	 *							0x0000000E	The system is shutting down; this function cannot be called.
	*/
	static int getServiceStatus(String^ svcName);
};

