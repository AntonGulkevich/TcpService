#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <mutex>
#include <map>
#include <atomic>
#include <iterator>

#include "Singleton.hpp"

#include "../SmallWebProtocols/TcpIOStream.h"
#include "../SmallWebProtocols/TcpConnector.h"
#include "../SmallWebProtocols/UdpMulticastStream.h"
#include "../SmallWebProtocols/Message.h"
#include "../../../SccLib/SccStaticLib/SccDeviceLib/SccDeviceLib.h"
#include "../../../SccLib/SccStaticLib/SccDeviceLib/SccTypes.h"

#define CHECK_CONNECTION_TIME_OUT_MILLESECONDS 300

typedef class SccMediator
{
private:
#pragma region Private members
	std::mutex _iostreamMapMutex;
	std::map<std::thread::id, std::unique_ptr<TcpIOStream>> _iostreamMap;
	LPUdpMulticastStream udpListener;

	// Mediator network setting
	LPCTSTR _strTCPHostName;
	LPCTSTR _strUDPHostName;
	USHORT _usTCPportNumber;
	USHORT _usUDPportNumber;

	// Events
	HANDLE _shutdownEvent;
	HANDLE _offlineTcpEvent;
	HANDLE _offlineUdpEvent;

	// Threads
	std::thread _reconnectionTcpThread;
	std::thread _reconnectionUdpThread;
	std::thread _deviceNotificationThread;

	// State of connection to the remote server
	std::atomic_bool _isTcpOnline;
	std::atomic_bool _isUdpOnline;

	// Recieve and send timeouts
	std::chrono::milliseconds _recvTimeout;
	std::chrono::milliseconds _sendTimeout;

#pragma endregion 

#pragma region Private functions

	void ReconnectTcp();
	void ReconnectUdp();

#pragma endregion 

#pragma region Helpers

	// If the function fails, the return value is error code
	// If the function succeeds, the return value is zero
	static INT CreateManualResetEvent(LPHANDLE eventHandle, BOOL initialState = FALSE);
	// Private ctor for singlton use only
	SccMediator();

	template <typename RetT, typename ... Args>
	RetT SendCommand(CommandHeader command, Args ...args);
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero or positive(count of bytes sended)
	INT Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds);
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is count of bytes received
	// If the connection has been gracefully closed, the return value is zero.
	INT Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds = INFINITE);
	// Remove IO Sthream from map
	void RemoveIOThread();
#pragma endregion 

#pragma region Initialization
	// If the function fails, the return value is error code
	// If the function succeeds, the return value is zero
	INT InitializeEvents();
	void CloseEvents() const;
	void StartReconnectionThreads();

#pragma endregion 
	// If the function fails, the return value is error code
	// If the function succeeds, the return value is zero
	INT CreateStreamIfNotExist();

	// If the function fails, the return value is error code
	// If the function succeeds, the return value is zero
	INT DeviceNotificationThread();
	std::map <std::thread::id, DeviceListChanged> deviceAddedHandlers;
	std::map <std::thread::id, DeviceListChanged> deviceRemovedHandlers;

public:
	// *-tors
	friend class Singleton<SccMediator>;
	~SccMediator();

	void SetServerInfo(LPCTSTR hostNameTCP, USHORT port, LPCTSTR hostNameUDP);
	INT Start();
	void Stop();
	// Return current state of UDP and TCP connection to remote server
	bool IsOnline();
	// Special holly sh***t implementation on raw pointers 
	void DisconnectCurrentClientFromServer();

#pragma region SccLib Functions Impl

	// return UINT on success, FALSE if channel is already in use, error code on failure 
	// bits in return value represent avaliable channels
	// 429[4] + 708[1] rest zero
	INT GetAvalibleChannelsIn(UINT deviceSerialNum);
	INT GetAvalibleChannelsOut(UINT deviceSerialNum);

	// Return TRUE on success, error code on failure
	INT ReleaseChannel429In(UINT deviceSerialNum, UINT channelNum);
	INT ReleaseChannel429Out(UINT deviceSerialNum, UINT channelNum);
	INT ReleaseChannel708In(UINT deviceSerialNum, UINT channelNum);
	INT ReleaseChannel708Out(UINT deviceSerialNum, UINT channelNum);

	// Return TRUE on success, error code on failure
	INT ConnectForced429In(UINT deviceSerialNum, UINT channelNum);
	INT ConnectForced429Out(UINT deviceSerialNum, UINT channelNum);
	INT ConnectForced708In(UINT deviceSerialNum, UINT channelNum);
	INT ConnectForced708Out(UINT deviceSerialNum, UINT channelNum);

	// If the function fails, the return value is negative value error code 
	// If the function succeeds, the return value is microsecconds count
	INT Ping();
	//Количество подключенных устройств
	INT GetDeviceCount();
	//Список серийный номеров подключенных устройств
	std::vector<UINT> GetDeviceNums();
	//Отправить список слов Arinc429 в формате структуры Arinc429 (время, данные)
	INT Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> &words);
	//Получить список принятых слов Arinc429 в формате структуры Arinc429 (время, данные)
	INT Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words);
	//Отправить список слов Arinc708 в формате структуры Arinc708 (время, данные(200 байт))
	INT Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> &words);
	//Получить список принятых слов Arinc708 в формате структуры Arinc708 (время, данные(200 байт))
	INT Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words);
	// Установить параметры входного канала Arinc429
	INT Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType = Arinc429ParityTypeIn::Analys);
	//Установить параметры выходного канала Arinc429
	INT Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType = Arinc429ParityTypeOut::Odd);
	//Количество неотправленных слов в выходном канале Arinc429
	INT Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	//Количество неотправленных слов в выходном канале Arinc708
	INT Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	//Суммарное время неотправленных слов в выходном канале Arinc429 в мкс
	INT Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
	// Суммарное время неотправленных слов в выходном канале Arinc708 в мкс
	INT Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
	//Установить максимальное количество слов во входном буфере канала Arinc429 (старые слова будут удаляться)
	INT Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
	//Установить максимальное количество слов во входном буфере канала Arinc708 (старые слова будут удаляться)
	INT Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
	//Очистка выходного буфера канала Arinc429
	// return If the function fails, the return value is negative error code.
	// return If the function succeeds, the return value is zero.
	INT ResetOut429Channel(UINT deviceSerialNum, UINT channelNum);
	//Очистка выходного буфера канала Arinc708
	INT ResetOut708Channel(UINT deviceSerialNum, UINT channelNum);
	// Очистка входного буфера канала Arinc429
	INT ResetIn429Channel(UINT deviceSerialNum, UINT channelNum);
	//Очистка входного буфера канала Arinc708
	INT ResetIn708Channel(UINT deviceSerialNum, UINT channelNum);
	//станавливает функцию для обработки события при появлении нового устройства
	void SetDeviceAttachedHandler(DeviceListChanged handler);
	//Устанавливает функцию для обработки события при пропадании устройства
	void SetDeviceDetachedHandler(DeviceListChanged handler);
	//Список серийный номеров подключенных устройств
	INT GetDeviceNumsRaw(UINT* nums, UINT max_size);
	// Получить конфигурацию кабеля подключенного к ППК
	INT GetPinConfiguration(UINT deviceSerialNum);
	// Получить версию библиотеки ППК
	INT GetDllVersion();
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero on disconnection or positive(count of bytes sended) on successfully sent
	template <typename Word>
	INT SendWords(UINT deviceSerialNum, UINT channelNum, const Word* words, UINT max_count);
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero or positive(count of bytes sended)
	template <typename Word>
	INT RecWords(UINT deviceSerialNum, UINT channelNum, Word* words, UINT max_count);
	// If the function fails, the return value is negative error Code
	// If the function succeeds, the return value is zero on disconnection or positive(count of words) on successfully receive
#pragma endregion 
} *LPSTMDATA;

/*
 * bytes order:
 * 0-3					packet size (uint)
 * 4-7					command number (uint)
 * 9-max_buffer_size	data
 */

template <typename RetT, typename ... Args>
RetT SccMediator::SendCommand(CommandHeader command, Args... args)
{
	Message rawData(command);
	rawData.Append(args...);
	auto resTcp = Send(rawData.GetRawData(), rawData.GetBytesCount(), _sendTimeout.count());
	if (resTcp > 0)
	{
		CHAR recb[sizeof(UINT)];
		resTcp = Recieve(recb, sizeof(UINT), _recvTimeout.count());
		if (resTcp > 0)
			return static_cast<INT>(Message::ReadPODFromBytes<RetT>(recb));
	}
	if (!SCC_ERROR(resTcp))
		RemoveIOThread();
	return resTcp;
}

template <typename Word>
INT SccMediator::SendWords(UINT deviceSerialNum, UINT channelNum, const  Word* words, UINT max_count)
{
	Message rawData(sizeof(Word) == sizeof(Word708) ? CommandHeader::Send708WordsRaw : CommandHeader::Send429WordsRaw);
	rawData.Append(deviceSerialNum, channelNum, max_count);
	rawData.Append(PCHAR(words), static_cast<size_t>(max_count) * sizeof(Word));

	// Send message
	auto resTcp = Send(rawData.GetRawData(), rawData.GetBytesCount(), _sendTimeout.count());
	if (resTcp > 0)
	{
		CHAR recb[sizeof(UINT)];
		// Recieve reply on success
		resTcp = Recieve(recb, sizeof(UINT), _recvTimeout.count());
		if (resTcp > 0)
			return Message::ReadPODFromBytes<INT>(reinterpret_cast<PNZCH>(recb), NULL);
		
	}
	if (!SCC_ERROR(resTcp))
		RemoveIOThread();
	return resTcp;
}

template <typename Word>
INT SccMediator::RecWords(UINT deviceSerialNum, UINT channelNum, Word* words, UINT max_count)
{
	// Create and fill message sent to server
	Message rawData(sizeof(Word) == sizeof(Word708) ? CommandHeader::Receive708WordsRaw : CommandHeader::Receive429WordsRaw);
	rawData.Append(deviceSerialNum, channelNum, max_count);
	// Send message
	auto resTcp = Send(rawData.GetRawData(), rawData.GetBytesCount(), _sendTimeout.count());
	if (resTcp > 0)
	{
		auto recb = new CHAR[MAX_BUFFER_LEN];
		// Recieve reply on success
		resTcp = Recieve(recb, MAX_BUFFER_LEN, _recvTimeout.count());
		if (resTcp > 0)
		{
			auto wordsCount = Message::ReadPODFromBytes<INT>(recb, 0);
			if (wordsCount > 0) {
				memcpy_s(words, wordsCount * sizeof(Word), recb + sizeof(UINT), wordsCount * sizeof(Word));
			}
			delete[]recb;
			return wordsCount;
		}
		delete[]recb;
	}
	if (!SCC_ERROR(resTcp))
		RemoveIOThread();
	return resTcp;
}

typedef Singleton<SccMediator> SccMediatorSingleton;
