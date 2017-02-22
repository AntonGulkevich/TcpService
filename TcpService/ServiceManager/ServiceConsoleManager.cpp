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

BOOL ServiceConsoleManager::InstallSvc(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription)
{
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(nullptr, szPath, MAX_PATH)) return FALSE;

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
		szPath, // path to service's binary 
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
	SERVICE_DESCRIPTION service_description { svcDecription };
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
	auto dwStartTime = GetTickCount();

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

		if (GetTickCount() - dwStartTime > dwMillisecondsTimeout)
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

		if (GetTickCount() - dwStartTime > dwMillisecondsTimeout)
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
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
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

	// Get service status information

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

	dwStartTickCount = GetTickCount();
	dwOldCheckPoint = ssStatus.dwCheckPoint;

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

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
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

	dwStartTickCount = GetTickCount();
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

			dwStartTickCount = GetTickCount();
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else
		{
			if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint)
			{
				// No progress made within the wait hint.
				break;
			}
		}
	}

	// Determine whether the service is running.

	//if (ssStatus.dwCurrentState == SERVICE_RUNNING)
	//{
	//	printf("Service started successfully.\n");
	//}
	//else
	//{
	//	printf("Service not started. \n");
	//	printf("  Current State: %d\n", ssStatus.dwCurrentState);
	//	printf("  Exit Code: %d\n", ssStatus.dwWin32ExitCode);
	//	printf("  Check Point: %d\n", ssStatus.dwCheckPoint);
	//	printf("  Wait Hint: %d\n", ssStatus.dwWaitHint);
	//}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
	return TRUE;
}

BOOL ServiceConsoleManager::StopDependentServices(SC_HANDLE schService, SC_HANDLE schSCManager)
{
	DWORD i;
	DWORD dwBytesNeeded;
	DWORD dwCount;

	LPENUM_SERVICE_STATUS lpDependencies = nullptr;
	ENUM_SERVICE_STATUS ess;
	SC_HANDLE hDepService;
	SERVICE_STATUS_PROCESS ssp;

	auto dwStartTime = GetTickCount();
	DWORD dwTimeout = 30000; // 30-second time-out

							 // Pass a zero-length buffer to get the required buffer size.
	if (EnumDependentServices(schService, SERVICE_ACTIVE,
		lpDependencies, 0, &dwBytesNeeded, &dwCount))
	{
		// If the Enum call succeeds, then there are no dependent
		// services, so do nothing.
		return TRUE;
	}
	else
	{
		if (GetLastError() != ERROR_MORE_DATA)
			return FALSE; // Unexpected error

						  // Allocate a buffer for the dependencies.
		lpDependencies = static_cast<LPENUM_SERVICE_STATUS>(HeapAlloc(
			GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesNeeded));

		if (!lpDependencies)
			return FALSE;

		__try
		{
			// Enumerate the dependencies.
			if (!EnumDependentServices(schService, SERVICE_ACTIVE,
				lpDependencies, dwBytesNeeded, &dwBytesNeeded,
				&dwCount))
				return FALSE;

			for (i = 0; i < dwCount; i++)
			{
				ess = *(lpDependencies + i);
				// Open the service.
				hDepService = OpenService(schSCManager,
					ess.lpServiceName,
					SERVICE_STOP | SERVICE_QUERY_STATUS);

				if (!hDepService)
					return FALSE;

				__try
				{
					// Recieve a stop code.
					if (!ControlService(hDepService,
						SERVICE_CONTROL_STOP,
						reinterpret_cast<LPSERVICE_STATUS>(&ssp)))
						return FALSE;

					// Wait for the service to stop.
					while (ssp.dwCurrentState != SERVICE_STOPPED)
					{
						Sleep(ssp.dwWaitHint);
						if (!QueryServiceStatusEx(
							hDepService,
							SC_STATUS_PROCESS_INFO,
							reinterpret_cast<LPBYTE>(&ssp),
							sizeof(SERVICE_STATUS_PROCESS),
							&dwBytesNeeded))
							return FALSE;

						if (ssp.dwCurrentState == SERVICE_STOPPED)
							break;

						if (GetTickCount() - dwStartTime > dwTimeout)
							return FALSE;
					}
				}
				__finally
				{
					// Always release the service handle.
					CloseServiceHandle(hDepService);
				}
			}
		}
		__finally
		{
			// Always free the enumeration buffer.
			HeapFree(GetProcessHeap(), 0, lpDependencies);
		}
	}
	return TRUE;
}
DWORD ServiceConsoleManager::getServiceStatus(LPCTSTR svcName)
{
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwBytesNeeded;

	auto schSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
	if (!schSCManager) return 0;

	auto schService = OpenService(schSCManager, svcName, SERVICE_ALL_ACCESS);
	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		return 0;
	}

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, LPBYTE(&ssStatus), sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded))
	{
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return 0;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return ssStatus.dwCurrentState;
}