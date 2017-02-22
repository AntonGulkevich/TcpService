#include "SccService.h"
#include <iostream>
#include "SccMediator.h"

SccService::SccService() : p_acceptor_(nullptr), stopped(false), acceptorThread()
{
}

SccService::~SccService()
{
	if (p_acceptor_ != nullptr)
		delete p_acceptor_;
	WSACleanup();
}

int SccService::Start(LPCTSTR address, USHORT port, int backlog)
{
	stopped = false;
	p_acceptor_ = new TcpAcceptor(address, port, backlog);

	if (p_acceptor_->Start() == NO_ERROR)
	{
		while (!stopped) {
			// New Connection
			auto stream = p_acceptor_->Accept();
			if (stream)
			{
				std::shared_ptr<StreamWorker> shpStrW(new StreamWorker, [=](StreamWorker* strW)
				{
					// custom deleter
					strW->ShutDown();
					delete strW;
				});
				if (shpStrW->SetInOut(stream))
				{
					shpStrW->AsyncRun();
					workers.push(std::move(shpStrW));
				}
			}
			else
			{
				// Unable to create connection with client
			}
		}
	}
	if (p_acceptor_ != nullptr)
		delete p_acceptor_;
	p_acceptor_ = nullptr;
	return NO_ERROR;
}

void SccService::AsyncStart(LPCTSTR address, USHORT port, int backlog)
{
	acceptorThread = std::thread(&SccService::Start, this, address, port, backlog);
}


int SccService::Stop()
{
	stopped = true;
	// wait for closing all connections 

	workers.clear();
	if (acceptorThread.joinable())
		acceptorThread.join();
	return NO_ERROR;
}

bool StreamWorker::SetInOut(PTcpIOStream iostream)
{
	p_stream_ = iostream;
	isEnable = (p_stream_ != nullptr);
	return isEnable;
}

void StreamWorker::AsyncRun()
{
	streamThread = std::thread(&StreamWorker::Run, this);
}

void StreamWorker::ShutDown()
{
	isEnable = false;
	std::unique_lock<std::mutex> lk(cv_m);
	ready2del.wait(lk);
	if (p_stream_)
		delete p_stream_;
	p_stream_ = nullptr;

}

void StreamWorker::Run()
{
	// Processing client stream
	do {
		auto portNumb = p_stream_->GetPortNumber();
		std::cout << portNumb << " from port. Recieving...\n";

		auto buf = new BYTE[MAX_MESSAGE_LENGTH];
		auto rSt = p_stream_->Recieve(PNZCH(buf), MAX_MESSAGE_LENGTH);

		switch (rSt)
		{
		case TIMEDOUT: std::cout << portNumb << " from port. Timed out\n";
			ShutDown();
			break;
		case SOCKET_ERROR:
			std::cout << portNumb << " from port. SOCKET_ERROR\n";
			ShutDown();
			break;
		case 0:
			std::cout << portNumb << " from port. Disconnected\n";
			ShutDown();
			break;
		default:
			std::cout << portNumb << " from port. Processing...\n";
			auto cropBuf = new BYTE[rSt];
			if (memcpy_s(cropBuf, rSt, buf, rSt) == NO_ERROR)
			{
				process(PNZCH(cropBuf), rSt);
				
			}
			else
			{
				// Handle memcpy_s errors
			}
			delete[] cropBuf;
		}
		delete[] buf;

	} while (isEnable);

	ready2del.notify_all();
}

void StreamWorker::process(PNZCH buf, UINT size) const
{
	//		/* Thread Pool:
	//		 * 1.Recieving thread:	1. recieve message 
	//		 *						2. move it to queue
	//		 *						3. continue recieveng
	//		 *						4. tell parsing thread that queue is not empty
	//		 * 2.Parsing thread:		1. wait condition change from recieving thread
	//		 *						2. while(queue is not empty) pop_first from queue 
	//		 *						3. split by commands
	//		 *						4. move command to commands queue
	//		 * 3,Command dispatcher thread:
	//		 *						1. pop_first from commands queue
	//		 *						2. process command
	//		 *						3. while (commands queue is not empty)
	//		 */
	Message m2s;
	m2s.Add(isEnable);

	auto sRes = p_stream_->Send(PCNZCH(m2s.GetRawData()), m2s.GetBytesCount());

	switch (sRes)
	{
	case TIMEDOUT:
		break;
	case SOCKET_ERROR:
		break;
	default:
		//sending is ok
		;
	}
}

StreamWorker::StreamWorker() : p_stream_(nullptr), streamThread(), isEnable(false)
{
}

StreamWorker::~StreamWorker()
{
	if (streamThread.joinable())
		streamThread.join();
	std::cout << "~StreamWorker\n";
}
