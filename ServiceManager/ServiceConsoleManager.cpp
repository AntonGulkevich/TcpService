#include "ServiceConsoleManager.h"

BOOL ServiceConsoleManager::DeleteSvc(LPCTSTR svcName)
{

	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager) return FALSE;

	auto schService = OpenService(schSCManager, svcName, DELETE);
	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}
	if (!DeleteService(schService))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;

}

BOOL ServiceConsoleManager::InstallSvcLocal(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription)
{
	TCHAR szPath[MAX_PATH];
	return GetModuleFileName(nullptr, szPath, MAX_PATH) ? InstallSvc(svcName, svcDisplayName, svcDecription, szPath) : FALSE;
}

BOOL ServiceConsoleManager::InstallSvc(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription, LPTSTR pathToBinary)
{
	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager) return FALSE;

	auto schService = CreateService(
		schSCManager, // SCM database 
		svcName, // name of service 
		svcDisplayName, // service name to display 
		SERVICE_ALL_ACCESS, // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START, // start type 
		SERVICE_ERROR_NORMAL, // error control type 
		pathToBinary, // path to service's binary 
		nullptr, // no load ordering group 
		nullptr, // no tag identifier 
		nullptr, // no dependencies 
		nullptr, // LocalSystem account 
		nullptr); // no password 

	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}
	SERVICE_DESCRIPTION service_description{ svcDecription };
	if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &service_description)) {
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

BOOL ServiceConsoleManager::StopSvc(LPCTSTR svcName, DWORD dwMillisecondsTimeout)
{
	auto dwStartTime = GetTickCount64();

	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

	if (!schSCManager) return FALSE;

	auto schService = OpenService(schSCManager, svcName,
		SERVICE_STOP |
		SERVICE_QUERY_STATUS |
		SERVICE_ENUMERATE_DEPENDENTS);

	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Make sure the service is not already stopped.
	DWORD dwBytesNeeded;
	SERVICE_STATUS_PROCESS ssp;

	if (!QueryServiceStatusEx(
		schService,
		SC_STATUS_PROCESS_INFO,
		reinterpret_cast<LPBYTE>(&ssp),
		sizeof(SERVICE_STATUS_PROCESS),
		&dwBytesNeeded))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	if (ssp.dwCurrentState == SERVICE_STOPPED)
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// If a stop is pending, wait for it.

	while (ssp.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		auto dwWaitTime = ssp.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&ssp),
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}

		if (GetTickCount64() - dwStartTime > dwMillisecondsTimeout)
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}
	}

	// If the service is running, dependencies must be stopped first.

	StopDependentServices(schService, schSCManager);

	// Recieve a stop code to the service.

	if (!ControlService(
		schService,
		SERVICE_CONTROL_STOP,
		reinterpret_cast<LPSERVICE_STATUS>(&ssp)))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Wait for the service to stop.

	while (ssp.dwCurrentState != SERVICE_STOPPED)
	{
		Sleep(ssp.dwWaitHint);
		if (!QueryServiceStatusEx(
			schService,
			SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&ssp),
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded))
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}

		if (ssp.dwCurrentState == SERVICE_STOPPED)
			break;

		if (GetTickCount64() - dwStartTime > dwMillisecondsTimeout)
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

BOOL ServiceConsoleManager::StartSvc(LPCTSTR svcName)
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwWaitTime;
	DWORD dwBytesNeeded;


	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

	if (nullptr == schSCManager) return FALSE;

	auto schService = OpenService(schSCManager, svcName, SERVICE_ALL_ACCESS);

	if (schService == nullptr)
	{
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Update service status information

	if (!QueryServiceStatusEx(
		schService, // handle to service 
		SC_STATUS_PROCESS_INFO, // information level
		reinterpret_cast<LPBYTE>(&ssStatus), // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded)) // size needed if buffer is too small
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Save the tick count and initial checkpoint.

	auto dwStartTickCount = GetTickCount64();
	auto dwOldCheckPoint = ssStatus.dwCheckPoint;

	// Wait for the service to stop before attempting to start it.

	while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth of the wait hint but not less than 1 second  
		// and not more than 10 seconds. 

		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(
			schService, // handle to service 
			SC_STATUS_PROCESS_INFO, // information level
			reinterpret_cast<LPBYTE>(&ssStatus), // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded)) // size needed if buffer is too small
		{
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return FALSE;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// SvcCtrlHandlerContinue to wait and check.

			dwStartTickCount = GetTickCount64();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				CloseServiceHandle(schService);
				CloseServiceHandle(schSCManager);
				return FALSE;
			}
		}
	}

	// Attempt to start the service.

	if (!StartService(
		schService, // handle to service 
		0, // number of arguments 
		nullptr)) // no arguments 
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Check the status until the service is no longer start pending. 
	if (!QueryServiceStatusEx(
		schService, // handle to service 
		SC_STATUS_PROCESS_INFO, // info level
		reinterpret_cast<LPBYTE>(&ssStatus), // address of structure
		sizeof(SERVICE_STATUS_PROCESS), // size of structure
		&dwBytesNeeded)) // if buffer too small
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return FALSE;
	}

	// Save the tick count and initial checkpoint.
	dwStartTickCount = GetTickCount64();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

	while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
	{
		// Do not wait longer than the wait hint. A good interval is 
		// one-tenth the wait hint, but no less than 1 second and no 
		// more than 10 seconds. 
		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000)
			dwWaitTime = 1000;
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);
		// Check the status again. 
		if (!QueryServiceStatusEx(
			schService, // handle to service 
			SC_STATUS_PROCESS_INFO, // info level
			reinterpret_cast<LPBYTE>(&ssStatus), // address of structure
			sizeof(SERVICE_STATUS_PROCESS), // size of structure
			&dwBytesNeeded)) // if buffer too small
		{
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint)
		{
			// SvcCtrlHandlerContinue to wait and check.

			dwStartTickCount = GetTickCount64();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}


	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}
/**
* \brief Return current state of the service
* \param svcName
* \return return current state of the service on success else return 0
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
*							0x0000000E  The specified database does not exist
*/
DWORD ServiceConsoleManager::getServiceStatus(LPCTSTR svcName)
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwBytesNeeded;

	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!schSCManager)
	{
		auto err = GetLastError();
		switch (err)
		{
		case ERROR_ACCESS_DENIED:
			return 0x00000008;
		case ERROR_DATABASE_DOES_NOT_EXIST:
			return 0x0000000F;
		default:
			return 0x00000009;
		}
	}

	auto schService = OpenService(schSCManager, svcName, SERVICE_ALL_ACCESS);
	if (!schService)
	{
		auto err = GetLastError();
		CloseServiceHandle(schSCManager);

		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		{
			return 0x00000008;
		}
		case ERROR_INVALID_HANDLE:
		{
			return 0x00000009;
		}
		case ERROR_INVALID_NAME:
		{
			return 0x0000000A;
		}
		case ERROR_SERVICE_DOES_NOT_EXIST:
		{
			return 0x0000000B;
		}
		default:
			return 0x00000009;
		}
	}

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, LPBYTE(&ssStatus), sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded))
	{
		auto err = GetLastError();

		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);

		switch (err)
		{
		case ERROR_ACCESS_DENIED:
		{
			return 0x00000008;
		}
		case ERROR_INVALID_HANDLE:
		{
			return 0x00000009;
		}
		case ERROR_INSUFFICIENT_BUFFER:
		{
			return 0x0000000C;
		}
		case ERROR_INVALID_LEVEL:
		{
			return 0x0000000D;
		}
		case ERROR_SHUTDOWN_IN_PROGRESS:
		{
			return 0x0000000E;
		}
		default:
			return 0x00000009;
		}
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return ssStatus.dwCurrentState;
}
