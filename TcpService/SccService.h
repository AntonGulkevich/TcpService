#pragma once

#include <thread>
#include <ppl.h>
#include <concurrent_queue.h>
#include <memory>

#include "TcpAcceptor.h"
#include "TcpIOStream.h"
#include "Message.h"
#include "SccDeviceLib.h"


class StreamWorker;
class SccService;
class Runnable;


class Runnable
{
public:
	virtual ~Runnable() {}
	static void Run_thread(void *args)
	{
		auto p_Runnable = static_cast<Runnable*>(args);
		p_Runnable->Run();
	}
protected:
	virtual void Run() = 0;
};


class StreamWorker : public Runnable
{
	PTcpIOStream p_stream_;
	std::thread streamThread;
	bool isEnable;
	std::condition_variable ready2del;

	void process(PNZCH buf, UINT size) const;
	std::mutex cv_m;

public:
	StreamWorker();
	~StreamWorker();

	bool SetInOut(PTcpIOStream iostream);
	void AsyncRun();
	void ShutDown();

protected:
	void Run() override;

};

class SccService
{
	concurrency::concurrent_queue<std::shared_ptr<StreamWorker>> workers;

	PTcpAcceptor p_acceptor_;
	bool stopped;
	std::thread acceptorThread;

public:
	SccService();
	~SccService();

	/**
	 * \brief
	 * \param address ex: _T(127.0.0.1)
	 * \param port
	 * \param backlog
	 * \return
	 */
	int Start(LPCTSTR address, USHORT port, int backlog);
	void AsyncStart(LPCTSTR address, USHORT port, int backlog);
	/**
	* \brief  Stop Service
	* \return NO_ERROR on success, error code on fail
	*/
	int Stop();

};
