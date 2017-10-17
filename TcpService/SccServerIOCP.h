#pragma once
#define ACCEPT_WAIT_TIMEOUT 100
#define THREADS_PER_PROCESS 2 
#define _DEBUGOUTPUT

#define OP_READ     0
#define OP_WRITE    1

#include <winsock2.h>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <array>
#include <strsafe.h>

#include "ClientContext.h"
#include "VirtualDevice.h"
#include "../SmallWebProtocols/UdpMulticastStream.h"
#include "../SccMediatorLib/Singleton.hpp"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

typedef class SccServerIOCP final // fantasy XIV http://store.steampowered.com/app/39210/FINAL_FANTASY_XIV_Online/
{
	// Service listening port number TCP
	USHORT _usTCPportNumber;
	// Udp port
	USHORT _usUDPportNumber;
	// Service multicast address
	LPCTSTR _strUDPHostName;
	// Ip address of the host in doted string
	PCHAR _hostIP;
	// Listening thread
	std::thread _acceptThread;
	// Listening socket
	SOCKET _listenSocket;
	// Service information
	SOCKADDR_IN _serverAddress;
	// Count of worker threads
	INT _workerThreadsCount;
	// Worker threads  vector - Capt.Ob.
	std::vector<std::thread> _workerThreadsVector;
	// Event used to stop service
	HANDLE _shutdownEvent;
	// Main IOCP
	HANDLE _iocpMain;
	// Event used for connection new client
	WSAEVENT _wsaAcceptEvent;
	// Map with connected and working clients. Key : clientId , value :  ClientContext
	std::map<size_t, std::unique_ptr<ClientContext>> _clientsMap;
	// Client sync
	std::mutex _clientMutex;
	// Device map, key is serialNumber 
	std::map<UINT, std::unique_ptr<VirtualDevice>> _devicesMap;
	// Devices map sync mutex
	std::mutex _devicesMapMutex;
	// Use to provide syncronizitaion while closing threads  
	std::mutex _threadClosing;
	// Stores the number of thread closed
	UINT _threadClosingCounter;
	//
	std::thread _deviceNotificationThread;
	//
	UdpMulticastStream * _UDPstream;

#ifdef _DEBUGOUTPUT
	std::mutex _debugCoutMutex;
	static void cout() { std::cout << std::endl; };

	// Output debug information
	template <typename Head, typename ...Tail>
	void cout(Head const & head, Tail const & ...tail);
	// Output debug information syncronized
	template <typename Head, typename ...Tail>
	void debugCoutSync(Head const &head, Tail const&...tail);

	static void wcout() { std::wcout << std::endl; };
	template <typename Head, typename ...Tail>
	void wcout(Head const & head, Tail const & ...tail);
	template <typename Head, typename ...Tail>
	void debugWCoutSync(Head const &head, Tail const&...tail);
#else
	template <typename Head, typename ... Tail>
	void debugCoutSync(Head const& head, Tail const&... tail) {}
	template <typename Head, typename ... Tail>
	void debugWCoutSync(Head const &head, Tail const&...tail) {}
#endif

#pragma region Initialization 
	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitializeEnvironment();

	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitializeIOCP();

	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitHostAddress();

	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitializeEvents();

	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitializeDeviceNotificationThread();

	//If the function fails, the return value is negative error code.
	//If the function succeeds, the return value zero.
	INT InitializeWSASocket();

	// If the function fails, the return value is negative error code
	// If the function succeeds, the return value is positive or zero 
	INT InitializeVirtualDevices();

	// Close open sockets and handles
	void DeInitialize();
#pragma endregion 

#pragma region Threads 

	// do lost of work
	UINT WorkerThread(INT nThreadNo);

	// Waits for connection request and call AcceptConnection on success
	void AcceptThread();

	// Adds new client
	void AcceptConnection();

#pragma endregion 

#pragma region Channel Manager

	// If the function fails, the return value is negative error code
	// If the function succeeds, the return value is positive or zero 
	INT AddDevice(const DWORD deviceSerialNumber);

	// If the function fails, the return value is negative error code
	// If the function succeeds, the return value is positive or zero 
	INT RemoveDevice(DWORD deviceSerialNumber);

	// Return TRUE on success, error code on failure
	BOOL IsChannelAvaliableFor(DWORD deviceSerialNumber, ChannelTypeFull type, UINT channelNumber, size_t ownerId);

	// Return INT, bits in one mean is channel avaliable or not
	// 1 - is avaliable, 0 - is aready on use
	// first 4 bits bits are 1-4 numbers of 429 channels
	// 5th bit is 708 channel
	// If the function fails, the return value is negative error code
	// If the function succeeds, the return value is in range [0..31]
	INT GetChannelSatesIn(DWORD deviceSerialNumber, size_t ownerID);
	INT GetChannelSatesOut(DWORD deviceSerialNumber, size_t ownerID);

	// Return TRUE on success, error code on failure
	template<typename Type>
	BOOL ReleaseChannelIfAvaliableFor(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, Type channelType);

	// Return TRUE on success, error code on failure
	template<typename Type>
	BOOL ConnectForced(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, Type channelType);

	// Reset buffer if its avaliable for ownerID
	// Return TRUE on success, error code on failure
	BOOL ResetBuffer(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, ChannelTypeFull type);

#pragma endregion 

#pragma region Service Manager

	//Store client context in _clientsMap
	bool AddToClientList(std::unique_ptr<ClientContext> ClientContext);

	// Remove client content from _clientsMap and clear all information about client in virtual device
	UINT RemoveClient(size_t clientID);

	// Erase all information about all clients
	void CleanClientList();

	// Device notification events
	static void OnDeviceAttached(DWORD deviceSerialNumber);
	static void OnDeviceDetached(DWORD deviceSerialNumber);

#pragma endregion 

#pragma region Processing Command
	template <typename Out, typename ... Ts, std::size_t ... Is>
	Out PICHelper(
		PClientContext pClientContext,
		DWORD& bytesProcessed,
		const std::function<Out(Ts ...)>& function,
		const std::vector<UINT>& params,
		std::index_sequence<Is...> const&);

	template<typename Out, typename ...Types>
	void ProcessIncomingCommand(PClientContext pClientContext,
		DWORD & bytesProcessed,
		const std::function<Out(Types...)> &function,
		const std::vector<UINT> &params,
		bool premoderated = false,
		ChannelTypeFull type = Ignored);
#pragma endregion 

#pragma region Singleton

	static  SccServerIOCP* _serverInstPtr;
	friend class Singleton<SccServerIOCP>;
	SccServerIOCP();

#pragma endregion 

#pragma region Logging

	static void AddToLog(LPCTSTR message);
	void AddErrorLogEntry(PTSTR pszFunction, INT dwError);
	void AddEventLogEntry(LPCTSTR message, WORD type);

#pragma endregion 

	static UINT GetHardwareConcurrency() noexcept;

public:

	bool Stop();
	// If the function fails, the return value is negative error code.
	// If the function succeeds, the return value is zero.
	INT Start(LPCTSTR udpHost, USHORT port);

	~SccServerIOCP();
} *LPSccServerIOCP;

#ifdef _DEBUGOUTPUT
template <typename Head, typename ... Tail>
void SccServerIOCP::cout(Head const& head, Tail const&... tail)
{
	std::cout << head;
	cout(tail...);
}

template <typename Head, typename ... Tail>
void SccServerIOCP::debugCoutSync(Head const& head, Tail const&... tail)
{
	std::lock_guard<std::mutex>lock(_debugCoutMutex);
	cout(head, tail...);
}

template <typename Head, typename ... Tail>
void SccServerIOCP::wcout(Head const& head, Tail const&... tail)
{
	std::wcout << head;
	wcout(tail...);
}

template <typename Head, typename ... Tail>
void SccServerIOCP::debugWCoutSync(Head const& head, Tail const&... tail)
{
	std::lock_guard<std::mutex>lock(_debugCoutMutex);
	wcout(head, tail...);
}
#endif
template <typename Type>
BOOL SccServerIOCP::ReleaseChannelIfAvaliableFor(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, Type channelType)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto currentDevice = _devicesMap.find(deviceSerialNumber);
	if (currentDevice == _devicesMap.end())
		return DEVICE_NOT_AVAILABLE;
	auto retRes = currentDevice->second->IsAvaliableFor(channelType, channelNumber, ownerID);
	if (retRes != TRUE)
		return retRes;
	return currentDevice->second->SetOwner(channelType, channelNumber, NULL);
}

template <typename Type>
BOOL SccServerIOCP::ConnectForced(DWORD deviceSerialNumber, size_t ownerID, UINT channelNumber, Type channelType)
{
	std::lock_guard<std::mutex> lock(_devicesMapMutex);
	auto currentDevice = _devicesMap.find(deviceSerialNumber);
	if (currentDevice == _devicesMap.end())
		return DEVICE_NOT_AVAILABLE;
	return	currentDevice->second->SetOwner(channelType, channelNumber, ownerID);
}

template <typename Out, typename ... Ts, std::size_t... Is>
Out SccServerIOCP::PICHelper(PClientContext pClientContext, DWORD& bytesProcessed,
	const std::function<Out(Ts ...)>& function, const std::vector<UINT>& params, std::index_sequence<Is...> const&)
{
	Out resFromFunction = function(static_cast<Ts>(params.at(Is))...);
	return resFromFunction;
}

template <typename Out, typename ... Types>
void SccServerIOCP::ProcessIncomingCommand(PClientContext pClientContext, DWORD& bytesProcessed,
	const std::function<Out(Types...)>& function, const std::vector<UINT>& params, bool premoderated, ChannelTypeFull type)
{
	if (premoderated)
	{
		auto avaliableRet = IsChannelAvaliableFor(params.at(0), type, params.at(1), pClientContext->GetId());
		if (avaliableRet == TRUE)
		{
			auto conRes = ConnectForced(params.at(0), pClientContext->GetId(), params.at(1), type);
			if (conRes == TRUE)
			{
				Out resFromSccLib = PICHelper(pClientContext, bytesProcessed, function, params,
					std::make_index_sequence<sizeof...(Types)>());
				pClientContext->SetBufferT(resFromSccLib);
			}
			else
			{
				pClientContext->SetBufferT(conRes);
			}
		}
		else
			pClientContext->SetBufferT(static_cast<Out>(avaliableRet));
	}
	else
	{
		Out resFromSccLib = PICHelper(pClientContext, bytesProcessed, function, params,
			std::make_index_sequence<sizeof...(Types)>());
		pClientContext->SetBufferT(resFromSccLib);
	}
	bytesProcessed = sizeof(Out);
}
typedef Singleton<SccServerIOCP> SccServerIOCPSingleton;

