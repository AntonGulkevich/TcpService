#include "SccMediator.h"

SccMediator::SccMediator()
{
	/*
	 * Establishing connection
	 * Inform server about new client pending virtual device
	 * Prepare client create one connection
	*/
	
	if (Ping() == 0 )
	{
		isOnline = false;
	}
	else
	{
		isOnline = true;
	}
}

SccMediator::~SccMediator() {}

UINT SccMediator::Ping()
{
	std::lock_guard<std::mutex> lock(tcpMutex);
		auto stream = OpenStream();
	if (stream == nullptr) {
		return 0;
	}
 	Message rawData(static_cast<UINT>(Command::Ping));
	auto timeSta = std::chrono::system_clock::now();
	auto mcSec = std::chrono::duration_cast<std::chrono::microseconds>(timeSta.time_since_epoch());
	auto csdc = mcSec.count();
	rawData.Add(csdc);
	if (stream->Send(reinterpret_cast<PCHAR>(rawData.GetRawData()), rawData.GetBytesCount()) > 0) {
		BYTE recb[MAX_MESSAGE_LENGTH];
		auto resCount = stream->Recieve(reinterpret_cast<PCHAR>(recb), rawData.GetBytesCount(), 100);
		if (resCount > 0)
		{
			bool state = recb[resCount -1];
			if (!state)
			{
				threadsMap.erase(std::this_thread::get_id());
				return 0;
			}
			
 			auto endTime = std::chrono::system_clock::now();
			auto mcSecEnd = std::chrono::duration_cast<std::chrono::microseconds>(endTime.time_since_epoch());
			auto elapsedMcsec = std::chrono::duration_cast<std::chrono::microseconds>(mcSecEnd - mcSec).count();
 			return  elapsedMcsec;
		}
	}
	threadsMap.erase(std::this_thread::get_id());
	return 0;
}

PTcpIOStream SccMediator::OpenStream()
{
	auto thisThreadId = std::this_thread::get_id();
	auto it = threadsMap.find(thisThreadId);
	if (it == threadsMap.end())
	{
		auto tcpStream = TcpConnector::connect(HOSTNAME, PORTNUMBER);
		if (tcpStream == nullptr) return nullptr;
		threadsMap.insert(std::pair<std::thread::id, PTcpIOStream>(thisThreadId, tcpStream));
		return tcpStream;
	}
	return it->second;
}

INT SccMediator::GetDeviceCount()
{
	return INT(UintPrototype(Command::GetDeviceNums));
}

std::vector<UINT> SccMediator::GetDeviceNums()
{
	//std::lock_guard<std::mutex> lock(tcpMutex);
	//std::vector<UINT> result;
	//auto stream = OpenStream();
	//if (stream) {
	//	Message dataToSend(static_cast<UINT>(Command::GetDeviceNums));
	//	if (stream->Send(reinterpret_cast<PCHAR>(dataToSend.GetRawData()), dataToSend.GetBytesCount()) > 0) {
	//		BYTE recb[MAX_MESSAGE_LENGTH];
	//		auto recievedBytes = stream->Recieve(reinterpret_cast<PCHAR>(recb), MAX_MESSAGE_LENGTH);
	//		if (recievedBytes > 0)
	//		{
	//			for (auto i = 0; i < recievedBytes; i += sizeof(UINT))
	//				result.push_back(Message::ReadUint(recb, i));
	//			if (result[0] == 0)
	//				result.clear();
	//		}
	//	}
	//}
	//return result;
	return std::vector<UINT>();
}

int SccMediator::Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> words) const
{
	return SOCKET_ERROR;
}

int SccMediator::Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words)
{
	return SOCKET_ERROR;
}

int SccMediator::Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> words)
{
	return SOCKET_ERROR;
}

int SccMediator::Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words)
{
	return SOCKET_ERROR;
}

int SccMediator::Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType)
{
	return INT(UintPrototype(Command::Set429InputChannelParams, deviceSerialNum, channelNum, static_cast<UINT>(rate), static_cast<UINT>(parityType)));
}

int SccMediator::Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType)
{
	return INT(UintPrototype(Command::Set429OutputChannelParams, deviceSerialNum, channelNum, static_cast<UINT>(rate), static_cast<UINT>(parityType)));
}

size_t SccMediator::Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
{
	return UintPrototype(Command::Get429OutputBufferWordsCount, deviceSerialNum, channelNum);
}

size_t SccMediator::Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
{
	return UintPrototype(Command::Get708OutputBufferWordsCount, deviceSerialNum, channelNum);
}

UINT SccMediator::Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
{
	return UintPrototype(Command::Get429OutputBufferMicroseconds, deviceSerialNum, channelNum);
}

UINT SccMediator::Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
{
	return UintPrototype(Command::Get708OutputBufferMicroseconds, deviceSerialNum, channelNum);
}

INT SccMediator::Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
{
	return INT(UintPrototype(Command::Set429InputBufferLength, deviceSerialNum, channelNum, wordsCount));
}

INT SccMediator::Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
{
	return INT(UintPrototype(Command::Set708InputBufferLength, deviceSerialNum, channelNum, wordsCount));
}

INT SccMediator::ResetOut429Channel(UINT deviceSerialNum, UINT channelNum)
{
	return INT(UintPrototype(Command::ResetOut429Channel, deviceSerialNum, channelNum));
}

INT SccMediator::ResetOut708Channel(UINT deviceSerialNum, UINT channelNum)
{
	return INT(UintPrototype(Command::ResetOut708Channel, deviceSerialNum, channelNum));
}

INT SccMediator::ResetIn429Channel(UINT deviceSerialNum, UINT channelNum)
{
	return INT(UintPrototype(Command::ResetIn429Channel, deviceSerialNum, channelNum));
}

INT SccMediator::ResetIn708Channel(UINT deviceSerialNum, UINT channelNum)
{
	return INT(UintPrototype(Command::ResetIn708Channel, deviceSerialNum, channelNum));
}

void SccMediator::SetDeviceAttachedHandler(DeviceListChanged handler)
{
}

void SccMediator::SetDeviceDetachedHandler(DeviceListChanged handler)
{
}

void SccMediator::SetTraceHandler(Trace handler)
{
}

INT SccMediator::GetDeviceNumsRaw(UINT* nums, UINT max_size)
{
	return 0;
}

INT SccMediator::Send429WordsRaw(UINT deviceSerialNum, UINT channelNum, const Word429* words, size_t count)
{
	return 0;
}

INT SccMediator::Receive429WordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, size_t count)
{
	return 0;
}

INT SccMediator::Send708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, size_t count)
{
	return 0;
}

INT SccMediator::Receive708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, size_t max_count)
{
	return 0;
}


