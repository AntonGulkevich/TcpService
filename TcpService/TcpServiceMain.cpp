#pragma region Additional libs
#pragma comment(lib, "advapi32.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment( lib, "SccDeviceLib.lib" )
#pragma endregion

#include <tchar.h>
#include <iostream>
#include <string>
#include <fstream>
#include <WinSock2.h>

#include "ServiceManager/ServiceConsoleManager.h"
#include "ServiceManager/ProxyService.h"
#include "SccService.h"
#include "SccServerNPIO.h"

#pragma region Service parameters
#define SVCNAME _T("SccLibProxy")
#define SVCDISPLAYNAME _T("SccLib Proxy Service")
#define SVCDESCR _T("Provide the usage of SccLib via TCP")
#pragma endregion

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return WSAGetLastError();
	}

#ifdef SERVICE_MODE
	if (argc == 2) {
		if (lstrcmpi(argv[1], _T("install")) == 0)
		{
			if (ServiceConsoleManager::InstallSvc(SVCNAME, SVCDISPLAYNAME, SVCDESCR))
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

	// Initialize service and run it
	ProxyService prxSvc(SVCNAME);
	ServiceDispatcher::Run(prxSvc);

#else
	//SccService service;
	//service.AsyncStart(_T("127.0.0.1"), 1488, 256);
	SccServerNPIO serverNPIO;
	if (!serverNPIO.Start()) std::cout << "Server initialization fatal." << std::endl;
	system("pause");
	serverNPIO.Stop();
#endif
	return 0;
}