#pragma once
#include  <tchar.h>

#include "ServiceDispatcher.h"
#include "ThreadPool.h"
#include "../SccServerIOCP.h"

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
