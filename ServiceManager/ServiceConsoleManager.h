#pragma once
#include "WinSock2.h"
#include "Windows.h"

#pragma comment(lib, "Advapi32.lib")

class  ServiceConsoleManager final
{
public:
	/**
	 * \brief Delete service from SCM
	 * \param svcName 
	 * \return TRUE on success else return FALSE
	 */
	static BOOL WINAPI DeleteSvc(LPCTSTR svcName);
	/**
	 * \brief Install new service 
	 * \param svcName
	 * \param svcDisplayName
	 * \param svcDecription
	 * \return TRUE on success else return FALSE
	 */
	static BOOL WINAPI InstallSvcLocal(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription);

	static BOOL WINAPI InstallSvc(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription, LPTSTR pathToBinary);
	

	/**
	 * \brief Stop service and all dependent services with default timeout of 30 seconds
	 * \param svcName
	 * \param dwMillisecondsTimeout
	 * \return TRUE on success else return FALSE
	 */
	static BOOL WINAPI StopSvc(LPCTSTR svcName, DWORD dwMillisecondsTimeout = 30000);

	/**
	 * \brief Start installed service 
	 * \param svcName
	 * \return TRUE on success else return FALSE
	 */
	static BOOL WINAPI StartSvc(LPCTSTR svcName);

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
	 */
	static DWORD WINAPI getServiceStatus(LPCTSTR svcName);
private:
	/**
	 * \brief Stop all dependent services 
	 * \param schService 
	 * \param schSCManager 
	 * \return TRUE on success else return FALSE
	 */
	static BOOL WINAPI StopDependentServices(SC_HANDLE schService, SC_HANDLE schSCManager)
	{
		DWORD dwBytesNeeded;
		DWORD dwCount;

		LPENUM_SERVICE_STATUS lpDependencies = nullptr;
		ENUM_SERVICE_STATUS ess;
		SC_HANDLE hDepService;
		SERVICE_STATUS_PROCESS ssp;

		auto dwStartTime = GetTickCount64();
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
				GetProcessHeap(), HEAP_ZERO_MEMORY, static_cast<size_t>(dwBytesNeeded)));

			if (!lpDependencies)
				return FALSE;

			__try
			{
				// Enumerate the dependencies.
				if (!EnumDependentServices(schService, SERVICE_ACTIVE,
					lpDependencies, dwBytesNeeded, &dwBytesNeeded,
					&dwCount))
					__leave;

				for (size_t i = 0; i < dwCount; i++)
				{
					ess = *(lpDependencies + i);
					// Open the service.
					hDepService = OpenService(schSCManager,
						ess.lpServiceName,
						SERVICE_STOP | SERVICE_QUERY_STATUS);

					if (!hDepService)
						__leave;

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
								__leave;

							if (ssp.dwCurrentState == SERVICE_STOPPED)
								break;

							if (GetTickCount64() - dwStartTime > dwTimeout)
								__leave;
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
	ServiceConsoleManager() {};
	~ServiceConsoleManager() {};

};
