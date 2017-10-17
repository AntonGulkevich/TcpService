#define WIN32_LEAN_AND_MEAN

#include <tchar.h>
#include <iostream>
#include <string>

//#define SERVICE_MODE

// Used for basic operations with windows service
#include "../ServiceManager/ServiceConsoleManager.h"
// Implementation of Scc Service via TCP using Scc Server
#include "ServiceManager/ProxyService.h" 
// Implementation of Server
#include "SccServerIOCP.h"

// Turn on service mode, off - console application
#ifndef  SERVICE_MODE

#else
#pragma region Service parameters
#define SVCNAME _T("SccLibProxy")
#define SVCDISPLAYNAME _T("SccLib Proxy Service")
#define SVCDESCR _T("Provide the usage of SccLib via TCP")
#pragma endregion

#endif
#ifdef _WIN64
	#pragma comment (lib, "SccDeviceLib64.lib")
#else 
	#pragma comment (lib, "SccDeviceLib86.lib")
#endif

void __cdecl _tmain(int argc, TCHAR* argv[])
{
#ifdef SERVICE_MODE
	if (argc == 2) {
		if (lstrcmpi(argv[1], _T("install")) == 0)
		{
			if (ServiceConsoleManager::InstallSvcLocal(SVCNAME, SVCDISPLAYNAME, SVCDESCR))
				std::cout << "Service Installed.\n";
			else
				std::cout << "Failed to install service!.\n";
			return;
		}

		if (lstrcmpi(argv[1], _T("del")) == 0)
		{

			if (ServiceConsoleManager::DeleteSvc(SVCNAME))
				std::cout << "Service deleted.\n";
			else
				std::cout << "Failed to delete service!.\n";
			return;
		}

		if (lstrcmpi(argv[1], _T("stop")) == 0)
		{
			if (ServiceConsoleManager::StopSvc(SVCNAME))
				std::cout << "Service stopped.\n";
			else
				std::cout << "Failed to stop service!.\n";
			return;
		}

		if (lstrcmpi(argv[1], _T("start")) == 0)
		{
			if (ServiceConsoleManager::StartSvc(SVCNAME))
				std::cout << "Service started.\n";
			else
				std::cout << "Failed to start service!.\n";
			return;
		}
	}
	/*Entry point of the service*/

	// Check service installation 
	if (ServiceConsoleManager::getServiceStatus(SVCNAME) == 0) return;

	// InitializeSender service and run it
	ProxyService prxSvc(SVCNAME);
	ServiceDispatcher::Run(prxSvc);

#else
	auto  scc_server_iocp = &SccServerIOCPSingleton::Instance();
	auto errStart = scc_server_iocp->Start(nullptr, DEFAULT_PORT);
	if (errStart)
		std::cout << "Server initialization failed. Error code: " << errStart <<  std::endl;
	//press any key to stop service
	system("pause");
	if (scc_server_iocp->Stop())
		std::cout << "Server stopped.\n";
	system("pause");
#endif
}