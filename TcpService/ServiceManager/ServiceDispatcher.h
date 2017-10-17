#pragma once
#include "WinSock2.h"
#include "Windows.h"

class ServiceDispatcher
{
	static ServiceDispatcher *dsp_svc;
	PTSTR svcName_;
	SERVICE_STATUS svcStatus;
	SERVICE_STATUS_HANDLE svcStatusHandle;
	static void WINAPI SvcMain();
	static void WINAPI SvcCtrlHandler(DWORD dwCtrl);
	void Start();
	void Pause();
	void Continue();
	void Shutdown();
	void Stop();
protected:
	virtual void OnStart(){};
	virtual void OnStop(){};
	virtual void OnPause(){};
	virtual void OnContinue(){};
	virtual void OnShutdown(){};
	void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
#pragma region event log
	/*
	* EVENTLOG_SUCCESS				Information event
	* EVENTLOG_AUDIT_FAILURE		Failure Audit event
	* EVENTLOG_AUDIT_SUCCESS		Success Audit event
	* EVENTLOG_ERROR_TYPE			Error event
	* EVENTLOG_INFORMATION_TYPE		Information event
	* EVENTLOG_WARNING_TYPE			Warning event
	*/
	void AddEventLogEntry(LPCTSTR message, WORD type) const;
	void AddErrorLogEntry(PTSTR pszFunction, DWORD dwError) const;
#pragma endregion 
public:
	static BOOL Run(ServiceDispatcher &srvDsp);
	explicit ServiceDispatcher(PTSTR svsName);
	virtual ~ServiceDispatcher(){}
};
