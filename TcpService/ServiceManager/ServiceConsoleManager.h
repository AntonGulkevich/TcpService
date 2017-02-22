#pragma once
#include <Windows.h>

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
	static BOOL WINAPI InstallSvc(LPCTSTR svcName, LPCTSTR svcDisplayName, LPTSTR svcDecription);

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
	static BOOL WINAPI StopDependentServices(SC_HANDLE schService, SC_HANDLE schSCManager);
	ServiceConsoleManager() {};
	~ServiceConsoleManager() {};

};
