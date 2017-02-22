#pragma region Additional libs
#pragma comment(lib, "advapi32.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "SccMediator.lib")
#pragma endregion

#include "TcpIOStream.h"
#include "TcpConnector.h"
#include  "SccMediator.h"
#include "SccTypes.h"

#include <iostream>
#include <string>
#include <tchar.h>
#include <mutex>

std::mutex ioMutex;

template <typename T>
void AsyncCout(T const& value)
{
	std::lock_guard<std::mutex> lock(ioMutex);
	std::cout << value;

}

void GetNum(SccMediator * med)
{
	int times = 1;
	while (times++)
	{
		if (times%15==0)
			Sleep(2000);
		Sleep(100);
		std::cout << med->Ping();
		/*med->Set429InputChannelParams(10, 11, Arinc429Rate::R100, Arinc429ParityTypeIn::NoChange);*/
		AsyncCout("thread\n");
	}
}

int main(void)
{
	Sleep(1000);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 1;
	}

	auto sccTcpMediator = &SccMediatorSingleton::Instance();

	std::thread thread(GetNum, std::ref(sccTcpMediator));
	std::thread thread2(GetNum, std::ref(sccTcpMediator));

	while (true)
	{
		//sccTcpMediator->Set429OutputChannelParams(10, 11, Arinc429Rate::R12_5, Arinc429ParityTypeOut::NoChange);
		std::cout << sccTcpMediator->Ping();

		AsyncCout("main\n");
		Sleep(50);
	}


	system("pause");

	return 0;
}
