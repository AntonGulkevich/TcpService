#include "SccMediator.h"
#include <iostream>
#include <minwinbase.h>
#include <Setupapi.h>

SccMediator::SccMediator() : udpListener(nullptr), _strTCPHostName(nullptr), _strUDPHostName(nullptr), _usTCPportNumber(0), _usUDPportNumber(0), _shutdownEvent(nullptr), _offlineTcpEvent(nullptr),
_offlineUdpEvent(nullptr), _isTcpOnline(false), _recvTimeout(std::chrono::milliseconds(300)), _sendTimeout(std::chrono::milliseconds(300))
{}

INT SccMediator::Send(PCNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	std::lock_guard<std::mutex> lock(_iostreamMapMutex);
	auto err = 0;
	auto thisThreadId = std::this_thread::get_id();
	auto it = _iostreamMap.find(thisThreadId);
	if (it == _iostreamMap.end())
	{
		err = CreateStreamIfNotExist();
		if (err)
			return err;
	}
	it = _iostreamMap.find(thisThreadId);
	return it->second->Send(pBuff, buffSize, milliseconds);
}

INT SccMediator::Recieve(PNZCH pBuff, UINT buffSize, DWORD milliseconds)
{
	std::lock_guard<std::mutex> lock(_iostreamMapMutex);
	auto err = 0;
	auto thisThreadId = std::this_thread::get_id();
	auto it = _iostreamMap.find(thisThreadId);
	if (it == _iostreamMap.end())
	{
		err = CreateStreamIfNotExist();
		if (err)
			return err;
	}
	it = _iostreamMap.find(thisThreadId);
	return it->second->Recieve(pBuff, buffSize, milliseconds);
}

void SccMediator::SetServerInfo(LPCTSTR hostNameTCP, USHORT port, LPCTSTR hostNameUDP)
{
	_usTCPportNumber = port;
	_usUDPportNumber = port + 1;
	_strTCPHostName = hostNameTCP;
	_strUDPHostName = hostNameUDP;
}

void SccMediator::RemoveIOThread()
{
	std::lock_guard<std::mutex> lock(_iostreamMapMutex);
	auto thisThreadId = std::this_thread::get_id();
	auto it = _iostreamMap.find(thisThreadId);
	if (it != _iostreamMap.end())
	{
		_iostreamMap.erase(thisThreadId);
		deviceRemovedHandlers.erase(thisThreadId);
		deviceAddedHandlers.erase(thisThreadId);
	}
}

INT SccMediator::DeviceNotificationThread()
{
	auto err = 0;
	CHAR recb[sizeof(UINT) * 3 + INET_ADDRSTRLEN];
	auto recIp = new CHAR[INET_ADDRSTRLEN];
	auto ipPr = new char[INET_ADDRSTRLEN];
	// This err is Invalid Parameter Handler Routine result
	// For correct additional information use GetLastError if wcstombs_s fails
	err = wcstombs_s(nullptr, ipPr, INET_ADDRSTRLEN, _strTCPHostName, INET_ADDRSTRLEN);
	if (err)
		err = GetLastError();
	else {
		// Listen untill client raise shutdown event
		while (WaitForSingleObject(_shutdownEvent, NULL) != WAIT_OBJECT_0)
		{
			// Blocks on receive for recv Timeout in milliseconds
			auto recBytes = udpListener->Recieve(recb, sizeof(UINT) * 3 + INET_ADDRSTRLEN, _recvTimeout.count());
			// On WSAETIMEDOUT check _shutdownEvent and repeate till it raised
			if (recBytes == (-WSAETIMEDOUT))
				continue;
			if (recBytes > 0)
			{
				auto offsetIndex = 0;
				auto size = Message::ReadPODFromBytes<UINT>(recb, offsetIndex);
				auto command = Message::ReadPODFromBytes<CommandHeader>(recb, offsetIndex += sizeof(UINT));
				auto deviceID = Message::ReadPODFromBytes<UINT>(recb, offsetIndex += sizeof(UINT));
				offsetIndex += sizeof(UINT);
				if (memcpy_s(recIp, INET_ADDRSTRLEN, recb + offsetIndex, recBytes - offsetIndex))
				{
					err = GetLastError();
					break;
				}
				// If recieved ip address is the same as expected raise callback 
				// Else ignore Add/Dell device event from remote server
				if (strstr(ipPr, recIp)) {
					switch (command)
					{
					case CommandHeader::Attached: {
						for (auto added_handler : deviceAddedHandlers)
							added_handler.second(deviceID);
						break;
					}
					case CommandHeader::Detached: {
						for (auto added_handler : deviceRemovedHandlers)
							added_handler.second(deviceID);
						break;
					}
					default:
						//TODO: handle wrong command parameter
						err = INVALID_HEADER_DATA;
					}
				}
			}
			if (err)
				break;
		}
	}
	delete[]recIp;
	delete udpListener;
	udpListener = nullptr;
	_isUdpOnline = false;
	if (!SetEvent(_offlineUdpEvent))
		err = GetLastError();
	return err;
}

INT SccMediator::InitializeEvents()
{
	auto err = CreateManualResetEvent(&_offlineUdpEvent, TRUE);
	if (err)
		return err;
	err = CreateManualResetEvent(&_offlineTcpEvent, TRUE);
	if (err)
		return err;
	err = CreateManualResetEvent(&_shutdownEvent, FALSE);
	return err;
}

void SccMediator::CloseEvents() const
{
	if (_shutdownEvent != nullptr)
		CloseHandle(_shutdownEvent);
	if (_offlineUdpEvent != nullptr)
		CloseHandle(_offlineUdpEvent);
	if (_offlineTcpEvent != nullptr)
		CloseHandle(_offlineTcpEvent);
}

void SccMediator::StartReconnectionThreads()
{
	// Start TCP thread
	_reconnectionTcpThread = std::thread(&SccMediator::ReconnectTcp, this);
	// Start UDP thread
	_reconnectionUdpThread = std::thread(&SccMediator::ReconnectUdp, this);
}

bool SccMediator::IsOnline()
{
	return Ping() > 0 ? _isUdpOnline && _isTcpOnline : false;
}

void SccMediator::Stop()
{
	SetEvent(_shutdownEvent);
	SetEvent(_offlineTcpEvent);
	SetEvent(_offlineUdpEvent);

	if (_deviceNotificationThread.joinable())
		_deviceNotificationThread.join();
	if (_reconnectionTcpThread.joinable())
		_reconnectionTcpThread.join();
	if (_reconnectionUdpThread.joinable())
		_reconnectionUdpThread.join();

	_iostreamMap.clear();

	CloseEvents();
}

SccMediator::~SccMediator()
{
	Stop();
}

INT SccMediator::GetAvalibleChannelsIn(UINT deviceSerialNum)
{
	return SendCommand<INT>(CommandHeader::GetAvalibleChannelsIn, deviceSerialNum);
}

INT SccMediator::GetAvalibleChannelsOut(UINT deviceSerialNum)
{
	return  SendCommand<INT>(CommandHeader::GetAvalibleChannelsOut, deviceSerialNum);
}

INT SccMediator::ReleaseChannel429In(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ReleaseChannel429In, deviceSerialNum, channelNum);
}

INT SccMediator::ReleaseChannel429Out(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ReleaseChannel429Out, deviceSerialNum, channelNum);
}

INT SccMediator::ReleaseChannel708In(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ReleaseChannel708In, deviceSerialNum, channelNum);
}

INT SccMediator::ReleaseChannel708Out(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ReleaseChannel708Out, deviceSerialNum, channelNum);
}

INT SccMediator::ConnectForced429In(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ConnectForced429In, deviceSerialNum, channelNum);
}

INT SccMediator::ConnectForced429Out(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ConnectForced429Out, deviceSerialNum, channelNum);
}

INT SccMediator::ConnectForced708In(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ConnectForced708In, deviceSerialNum, channelNum);
}

INT SccMediator::ConnectForced708Out(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ConnectForced708Out, deviceSerialNum, channelNum);
}

INT SccMediator::Ping()
{
	auto timeStampBegin = std::chrono::system_clock::now();
	auto response = SendCommand<INT>(CommandHeader::Ping);
	return response == 0 ?
		static_cast<UINT>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timeStampBegin).count()) :
		response;
}

INT SccMediator::CreateManualResetEvent(LPHANDLE eventHandle, BOOL initialState)
{
	*eventHandle = CreateEvent(nullptr, TRUE, initialState, nullptr);
	auto err = GetLastError();
	if (err)
		return err;
	return NOERROR;
}

void SccMediator::ReconnectTcp()
{
	// On client shutdown
	while (WaitForSingleObject(_shutdownEvent, NULL) != WAIT_OBJECT_0)
	{
		while (WaitForSingleObject(_offlineTcpEvent, INFINITE) == WAIT_OBJECT_0)
		{
			if (WaitForSingleObject(_shutdownEvent, 300) == WAIT_OBJECT_0)
				break;
			// Try to connect to server using TCP
			auto err = 0;
			auto res = TcpConnector::connect(_strTCPHostName, _usTCPportNumber, &err);
			if (!err)
			{
				_isTcpOnline = true;
				ResetEvent(_offlineTcpEvent);
			}
		}
	}
}

void SccMediator::ReconnectUdp()
{
	// On client shutdown
	while (WaitForSingleObject(_shutdownEvent, NULL) != WAIT_OBJECT_0)
	{
		while (WaitForSingleObject(_offlineUdpEvent, INFINITE) == WAIT_OBJECT_0)
		{
			if (WaitForSingleObject(_shutdownEvent, 300) == WAIT_OBJECT_0)
				break;
			udpListener = UdpConnector::ConnectToMulticastGroup(_strUDPHostName, _usUDPportNumber);
			// Try to connect to server using UDP
			if (udpListener != nullptr)
			{
				_isUdpOnline = true;
				ResetEvent(_offlineUdpEvent);
				_deviceNotificationThread = std::thread(&SccMediator::DeviceNotificationThread, this);
			}
		}
	}
}

INT SccMediator::CreateStreamIfNotExist()
{
	if (!_isTcpOnline)
		return CONNECTION_LOST;
	auto err = 0;
	auto tcpStream = TcpConnector::connect(_strTCPHostName, _usTCPportNumber, &err);
	if (err)
	{
		_isTcpOnline = false;
		SetEvent(_offlineTcpEvent);
	}
	else
		_iostreamMap.insert(make_pair(std::this_thread::get_id(), std::move(tcpStream)));
	return err;
}

void SccMediator::DisconnectCurrentClientFromServer()
{
	// special magic numbers
	auto data = new CHAR[8];
	auto command = CommandHeader::Disconnect;
	auto size = 4;
	memcpy_s(data, 8, &size, 4);
	memcpy_s(data + 4, 8, &command, 4);
	Send(data, 8, _sendTimeout.count());
	delete[] data;
	RemoveIOThread();
}

INT SccMediator::Start()
{
	auto err = InitializeEvents();
	if (err)
		return err;

	StartReconnectionThreads();

	return err;
}

INT SccMediator::GetDeviceCount()
{
	return  SendCommand<INT>(CommandHeader::GetDeviceCount);
}

std::vector<UINT> SccMediator::GetDeviceNums()
{
	auto tempBuf = new UINT[256];
	auto resFromLib = GetDeviceNumsRaw(tempBuf, 256);
	std::vector<UINT> deviceSerialNumbers;
	if (resFromLib > 0)
	{
		for (auto i = 0; i < resFromLib; ++i)
			deviceSerialNumbers.emplace_back(tempBuf[i]);
	}
	delete[] tempBuf;
	return deviceSerialNumbers;
}

INT SccMediator::Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> &words)
{
	auto count = words.size() > MAX_WORD429_COUNT ? MAX_WORD429_COUNT : words.size();
	auto tempBuf = new Word429[count];
	auto endIt = words.begin();
	std::advance(endIt, count);
	std::copy(words.begin(), endIt, tempBuf);

	auto res = SendWords(deviceSerialNum, channelNum, tempBuf, count);
	delete[] tempBuf;
	return res;
}

INT SccMediator::Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words)
{
	auto tempBuf = new Word429[MAX_WORD429_COUNT];
	auto res = RecWords(deviceSerialNum, channelNum, tempBuf, MAX_WORD429_COUNT);
	if (res > 0)
		std::copy(tempBuf, tempBuf + res, std::back_inserter(words));
	delete[] tempBuf;
	return res;
}

INT SccMediator::Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> &words)
{
	auto count = words.size() > MAX_WORD708_COUNT ? MAX_WORD708_COUNT : words.size();
	auto tempBuf = new Word708[count];
	auto endIt = words.begin();
	std::advance(endIt, count);
	std::copy(words.begin(), endIt, tempBuf);
	auto res = SendWords(deviceSerialNum, channelNum, tempBuf, count);
	delete[] tempBuf;
	return res;
}

INT SccMediator::Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words)
{
	auto tempBuf = new Word708[MAX_WORD708_COUNT];
	auto res = RecWords(deviceSerialNum, channelNum, tempBuf, MAX_WORD708_COUNT);
	if (res > 0)
		std::copy(tempBuf, tempBuf + res, std::back_inserter(words));
	delete[] tempBuf;
	return res;
}

INT SccMediator::Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType)
{
	return  SendCommand<INT>(CommandHeader::Set429InputChannelParams, deviceSerialNum, channelNum, static_cast<UINT>(rate), static_cast<UINT>(parityType));
}

INT SccMediator::Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType)
{
	return  SendCommand<INT>(CommandHeader::Set429OutputChannelParams, deviceSerialNum, channelNum, static_cast<UINT>(rate), static_cast<UINT>(parityType));
}

INT SccMediator::Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::Get429OutputBufferWordsCount, deviceSerialNum, channelNum);
}

INT SccMediator::Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::Get708OutputBufferWordsCount, deviceSerialNum, channelNum);
}

INT SccMediator::Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::Get429OutputBufferMicroseconds, deviceSerialNum, channelNum);
}

INT SccMediator::Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::Get708OutputBufferMicroseconds, deviceSerialNum, channelNum);
}

INT SccMediator::Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
{
	return  SendCommand<INT>(CommandHeader::Set429InputBufferLength, deviceSerialNum, channelNum, wordsCount);
}

INT SccMediator::Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
{
	return  SendCommand<INT>(CommandHeader::Set708InputBufferLength, deviceSerialNum, channelNum, wordsCount);
}

INT SccMediator::ResetOut429Channel(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ResetOut429Channel, deviceSerialNum, channelNum);
}

INT SccMediator::ResetOut708Channel(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ResetOut708Channel, deviceSerialNum, channelNum);
}

INT SccMediator::ResetIn429Channel(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ResetIn429Channel, deviceSerialNum, channelNum);
}

INT SccMediator::ResetIn708Channel(UINT deviceSerialNum, UINT channelNum)
{
	return  SendCommand<INT>(CommandHeader::ResetIn708Channel, deviceSerialNum, channelNum);
}

void SccMediator::SetDeviceAttachedHandler(DeviceListChanged handler)
{
	deviceAddedHandlers.insert(std::make_pair(std::this_thread::get_id(), handler));
}

void SccMediator::SetDeviceDetachedHandler(DeviceListChanged handler)
{
	deviceRemovedHandlers.insert(std::make_pair(std::this_thread::get_id(), handler));
}

INT SccMediator::GetDeviceNumsRaw(UINT* nums, UINT max_size)
{
	Message rawData(CommandHeader::GetDeviceNumsRaw);
	rawData.Append(max_size);
	auto resTcp = Send(rawData.GetRawData(), rawData.GetBytesCount(), _sendTimeout.count());
	if (resTcp > 0)
	{
		CHAR recb[256];
		resTcp = Recieve(recb, 256, _recvTimeout.count());
		if (resTcp > NULL)
		{
			auto firstSn = Message::ReadPODFromBytes<INT>(recb);
			if (firstSn == 0)
				return firstSn;
			if (firstSn > 0) {
				if (!memcpy_s(nums, max_size * sizeof(INT), recb, resTcp))
					return resTcp / sizeof(INT);
				else
					resTcp = SccGetLastError;
			}
			if (firstSn < 0)
				return firstSn;
		}
	}
	if (!SCC_ERROR(resTcp))
		RemoveIOThread();
	return resTcp;
}

INT SccMediator::GetPinConfiguration(UINT deviceSerialNum)
{
	return  SendCommand<INT>(CommandHeader::GetPinConfiguration, deviceSerialNum);
}

INT SccMediator::GetDllVersion()
{
	return  SendCommand<UINT>(CommandHeader::GetDllVersion);
}


