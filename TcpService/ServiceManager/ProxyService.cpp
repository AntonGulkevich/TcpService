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
	SetEvent(prxSvcStopEvent);
	WaitForSingleObject(prxSvcStopEvent, 3000) ?
		AddErrorLogEntry(_T("Service stopped by timeout."), GetLastError()) :
		AddEventLogEntry(_T("Service stopped."), EVENTLOG_INFORMATION_TYPE);

}

void ProxyService::ServiceMainThread()
{
	/*start proxy server*/
	auto  scc_server_iocp = &SccServerIOCPSingleton::Instance();
	auto startRes = scc_server_iocp->Start(nullptr, DEFAULT_PORT);
	if (startRes) {
		AddErrorLogEntry(_T("Server initialization failed."), startRes);
		OnStop();
		return;
	}
	WaitForSingleObject(prxSvcStopEvent, INFINITE);
	scc_server_iocp->Stop();
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
