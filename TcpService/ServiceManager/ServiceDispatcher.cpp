#include "ServiceDispatcher.h"
#include <winbase.h>
#include <strsafe.h>

ServiceDispatcher * ServiceDispatcher::dsp_svc = nullptr;

void ServiceDispatcher::SvcMain()
{
	// Register the handler function for the service
	dsp_svc->svcStatusHandle = RegisterServiceCtrlHandler(
		dsp_svc->svcName_, SvcCtrlHandler);
	if (dsp_svc->svcStatusHandle == nullptr)
	{
		throw GetLastError();
	}

	// Start the service.
	dsp_svc->Start();
}

void ServiceDispatcher::SvcCtrlHandler(DWORD dwCtrl)
{
	switch (dwCtrl)
	{
		// Entrie system is shutting down.
	case SERVICE_CONTROL_SHUTDOWN: dsp_svc->Shutdown();	break;
		//continue
		/*After sending the stop request to a service, you should not send other controls to the service.*/
	case SERVICE_CONTROL_STOP: dsp_svc->Stop();
		break;
		/*Notifies a paused service that it should resume.
		*The hService handle must have the SERVICE_PAUSE_CONTINUE access right.*/
	case SERVICE_CONTROL_CONTINUE: dsp_svc->Continue();
		break;
		/*Notifies a service that it should report its current status information
		*to the service control manager. The hService handle must have
		*the SERVICE_INTERROGATE access right.
		*Note that this control is not generally useful as the SCM is aware
		*of the current state of the service.*/
	case SERVICE_CONTROL_INTERROGATE: break;
		/*Notifies a service that its startup parameters have changed.*/
	case SERVICE_CONTROL_PARAMCHANGE:
		break;
		/*Notifies a service that it should pause. */
	case SERVICE_CONTROL_PAUSE: dsp_svc->Pause();
		break;
	default:
		break;
	}
}

void ServiceDispatcher::Start()
{
	try
	{
		ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 0);
		OnStart();
		ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	}
	catch (DWORD dwError)
	{
		AddErrorLogEntry(L"Service Start", dwError);
		ReportSvcStatus(SERVICE_STOPPED, dwError, 0);
	}
	catch (...)
	{
		AddErrorLogEntry(L"Service failed to start.", EVENTLOG_ERROR_TYPE);
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
}

void ServiceDispatcher::Pause()
{
	try
	{
		ReportSvcStatus(SERVICE_PAUSE_PENDING, NO_ERROR, 0);
		OnPause();
		ReportSvcStatus(SERVICE_PAUSED, NO_ERROR, 0);
	}
	catch (DWORD dwError)
	{
		AddErrorLogEntry(L"Service Pause", dwError);
		ReportSvcStatus(SERVICE_RUNNING, dwError, 0);
	}
	catch (...)
	{
		AddErrorLogEntry(L"Service failed to pause.", EVENTLOG_ERROR_TYPE);
		ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	}
}

void ServiceDispatcher::Continue()
{
	try
	{
		ReportSvcStatus(SERVICE_CONTINUE_PENDING, NO_ERROR, 0);
		OnContinue();
		ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	}
	catch (DWORD dwError)
	{
		AddErrorLogEntry(L"Service Continue", dwError);
		ReportSvcStatus(SERVICE_PAUSED, dwError, 0);
	}
	catch (...)
	{
		AddErrorLogEntry(L"Service failed to resume.", EVENTLOG_ERROR_TYPE);
		ReportSvcStatus(SERVICE_PAUSED, NO_ERROR, 0);
	}
}

void ServiceDispatcher::Shutdown()
{
	try
	{
		OnShutdown();
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
	catch (DWORD dwError)
	{
		AddErrorLogEntry(L"Service Shutdown", dwError);
	}
	catch (...)
	{
		AddErrorLogEntry(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
	}
}

void ServiceDispatcher::Stop()
{
	try
	{
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		OnStop();
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
	catch (DWORD dwError)
	{
		AddErrorLogEntry(L"Service Stop", dwError);
		ReportSvcStatus(svcStatus.dwCurrentState, NO_ERROR, 0);
	}
	catch (...)
	{
		AddErrorLogEntry(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);
		ReportSvcStatus(svcStatus.dwCurrentState, NO_ERROR, 0);
	}
}

void ServiceDispatcher::ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;
	svcStatus.dwCurrentState = dwCurrentState;
	svcStatus.dwWin32ExitCode = dwWin32ExitCode;
	svcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		svcStatus.dwControlsAccepted = 0;
	else svcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		svcStatus.dwCheckPoint = 0;
	else svcStatus.dwCheckPoint = dwCheckPoint++;

	SetServiceStatus(svcStatusHandle, &svcStatus);
}

void ServiceDispatcher::AddEventLogEntry(LPCTSTR message, WORD type) const
{
	auto hEventLog = RegisterEventSource(nullptr, svcName_);
	//If the function fails, the return value is NULL.To get extended error information, call GetLastError.
	if (hEventLog == nullptr)
		return;
	LPCWSTR lpcwString[2] = { nullptr, nullptr };

	lpcwString[0] = svcName_;
	lpcwString[1] = message;

	ReportEvent(
		hEventLog, //A handle to the event log. 
		type,// The type of event to be logged				
		0, // Event category
		0, // Event identifier
		nullptr, //A pointer to the current user's security identifier. 
		2, // The number of insert strings in the array pointed to by the lpStrings parameter.
		0, // No binary data
		lpcwString,
		nullptr //A pointer to the buffer containing the binary data.
	);
	DeregisterEventSource(hEventLog);

}

void ServiceDispatcher::AddErrorLogEntry(PTSTR pszFunction, DWORD dwError) const
{
	wchar_t szMessage[260];
	StringCchPrintf(szMessage, ARRAYSIZE(szMessage),
		L"%s failed w/err 0x%08lx", pszFunction, dwError);
	AddEventLogEntry(szMessage, EVENTLOG_ERROR_TYPE);
}

BOOL ServiceDispatcher::Run(ServiceDispatcher& srvDsp)
{
	dsp_svc = &srvDsp;
	SERVICE_TABLE_ENTRY srvTableEntry[] = {
		{srvDsp.svcName_, LPSERVICE_MAIN_FUNCTION(SvcMain) },
		{nullptr, nullptr}
	};
	return StartServiceCtrlDispatcher(srvTableEntry);
}

ServiceDispatcher::ServiceDispatcher(PTSTR svsName) :
	svcName_(svsName),
	svcStatus({
		SERVICE_WIN32_OWN_PROCESS,	//dwServiceType
		SERVICE_START_PENDING,		//dwCurrentState
		SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PAUSE_CONTINUE,		//dwControlsAccepted
		NO_ERROR,					//dwWin32ExitCode
		0,					//dwServiceSpecificExitCode
		0,					//dwCheckPoint
		0 }),				//dwWaitHint
		svcStatusHandle(nullptr)
{

}
