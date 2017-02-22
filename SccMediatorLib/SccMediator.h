#pragma once
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <map>

#include "Message.h"
#include "TcpIOStream.h"
#include "TcpConnector.h"
#include "Singleton.hpp"
#include "SccTypes.h"

#define HOSTNAME _T("127.0.0.1")
#define PORTNUMBER 1488
#define MAX_MESSAGE_LENGTH 1024*65
#define HEADER_SIZE 8



//todo: multithreading: thread end event - close connection & remove map entry
#pragma region Commands
typedef enum class _commands
{
	Ping,
	GetDeviceCount,
	GetDeviceNums,
	Send429Words,
	Receive429Words,
	Send708Words,
	Receive708Words,
	Set429InputChannelParams,
	Set429OutputChannelParams,
	Get429OutputBufferWordsCount,
	Get708OutputBufferWordsCount,
	Get429OutputBufferMicroseconds,
	Get708OutputBufferMicroseconds,
	Set429InputBufferLength,
	Set708InputBufferLength,
	ResetOut429Channel,
	ResetOut708Channel,
	ResetIn429Channel,
	ResetIn708Channel,
	SetDeviceAttachedHandler,
	SetDeviceDetachedHandler,
	SetTraceHandler,
	GetDeviceNumsRaw,
	Send429WordsRawm,
	Receive429WordsRaw,
	Send708WordsRaw,
	Receive708WordsRaw,
	UnknownCommand
} Command;
#pragma endregion 

class SccMediator
{
private:
	std::mutex tcpMutex;
	std::map<std::thread::id, PTcpIOStream> threadsMap;

	bool isOnline;

	template <typename ... Args>
	void VoidPrototype(Command command, Args ...args);

	template <typename ... Args>
	UINT UintPrototype(Command command, Args ...args);
	PTcpIOStream OpenStream();
	SccMediator();
public:
	UINT Ping();
	~SccMediator();
	friend class Singleton<SccMediator>;

#pragma region SccLib Functions
	//Количество подключенных устройств
	INT GetDeviceCount();
	//Список серийный номеров подключенных устройств
	std::vector<UINT> GetDeviceNums();
	//Отправить список слов Arinc429 в формате структуры Word429 (время, данные)
	INT Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> words) const;
	//Получить список принятых слов Arinc429 в формате структуры Word429 (время, данные)
	INT Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words);
	//Отправить список слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
	INT Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> words);
	//Получить список принятых слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
	INT Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words);
	// Установить параметры входного канала Arinc429
	INT Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType = Arinc429ParityTypeIn::Analys);
	//Установить параметры выходного канала Arinc429
	INT Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType = Arinc429ParityTypeOut::Odd);
	//Количество неотправленных слов в выходном канале Arinc429
	size_t Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	//Количество неотправленных слов в выходном канале Arinc708
	size_t Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	//Суммарное время неотправленных слов в выходном канале Arinc429 в мкс
	UINT Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
	// Суммарное время неотправленных слов в выходном канале Arinc708 в мкс
	UINT Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);
	//Установить максимальное количество слов во входном буфере канала Arinc429 (старые слова будут удаляться)
	INT Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
	//Установить максимальное количество слов во входном буфере канала Arinc708 (старые слова будут удаляться)
	INT Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);
	//Очистка выходного буфера канала Arinc429
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
	// Устанавливает функцию для обработки события трассировки
	void SetTraceHandler(Trace handler);
	//Список серийный номеров подключенных устройств
	INT GetDeviceNumsRaw(UINT* nums, UINT max_size);
	// Отправить список слов Arinc429 в формате структуры Word429 (время, данные)
	INT Send429WordsRaw(UINT deviceSerialNum, UINT channelNum, const Word429* words, size_t count);
	//Получить список принятых слов Arinc429 в формате структуры Word429 (время, данные)
	INT Receive429WordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, size_t count);
	//Отправить список слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
	INT Send708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, size_t count);
	//Получить список принятых слов Arinc708 в формате структуры Word708 (время, данные(200 байт))
	INT Receive708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, size_t max_count);
#pragma endregion 
};

template <typename ... Args>
void SccMediator::VoidPrototype(Command command, Args ...args)
{
	std::lock_guard<std::mutex> lock(tcpMutex);
	auto stream = OpenStream();
	Message rawData(static_cast<UINT>(command));
	rawData.Add(args...);
	if (stream->Send(reinterpret_cast<PCHAR>(rawData.GetRawData()), rawData.GetBytesCount()) > 0) {
		//sending is ok
	}
	//SOCKET_ERROR
}

template <typename ... Args>
UINT SccMediator::UintPrototype(Command command, Args... args)
{
	std::lock_guard<std::mutex> lock(tcpMutex);
	auto stream = OpenStream();
	if (stream == nullptr) {
		return SOCKET_ERROR;
	}
	Message rawData(static_cast<UINT>(command));
	rawData.Add(args...);
	
	if (stream->Send(reinterpret_cast<PCHAR>(rawData.GetRawData()), rawData.GetBytesCount()) > 0) {
		auto recBSize = sizeof(UINT) * 2 + sizeof(BYTE);
		BYTE recb[9];
		auto resCount = stream->Recieve(reinterpret_cast<PCHAR>(recb), recBSize, 100);
		if (resCount > 0)
		{
			bool state = recb[recBSize-sizeof(BYTE)];
			if (!state)
			{
				threadsMap.erase(std::this_thread::get_id());
			}
			return  Message::ReadUint(recb, sizeof(UINT));
		}
	}
	threadsMap.erase(std::this_thread::get_id());

	return SOCKET_ERROR;
}

typedef Singleton<SccMediator> SccMediatorSingleton;
