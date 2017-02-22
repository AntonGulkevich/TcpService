#pragma once

#include <Windows.h>
#include <map> 
#include  <tchar.h>

#include "ServiceDispatcher.h"
#include "ThreadPool.h"


class ProxyService : public ServiceDispatcher
{
	HANDLE prxSvcStopEvent;
	BOOL stopped;
protected:
	void OnStart() override;
	void OnStop() override;

	void ServiceMainThread();
public:
	explicit ProxyService(LPTSTR name);
	~ProxyService();

};
