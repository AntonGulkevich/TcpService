#include "ProxyService.h"

void ProxyService::OnStart()
{
	prxSvcStopEvent = CreateEvent(nullptr, true, false, nullptr);
	stopped = false;
	CThreadPool::QueueUserWorkItem(&ProxyService::ServiceMainThread, this);
}

void ProxyService::OnStop()
{
	stopped = true;
	WaitForSingleObject(prxSvcStopEvent, 3000) ?
		AddErrorLogEntry(_T("Service stopped by timeout."), GetLastError()) :
		AddEventLogEntry(_T("Service stopped."), EVENTLOG_INFORMATION_TYPE);

}

void ProxyService::ServiceMainThread()
{
	/*start proxy server*/
	

	while (!stopped)
	{
		
	}

	SetEvent(prxSvcStopEvent);
}

ProxyService::ProxyService(LPTSTR name): ServiceDispatcher(name), prxSvcStopEvent(nullptr), stopped(false)
{
}

ProxyService::~ProxyService()
{
	if (prxSvcStopEvent)
	{
		CloseHandle(prxSvcStopEvent);
		prxSvcStopEvent = nullptr;
	}
}
