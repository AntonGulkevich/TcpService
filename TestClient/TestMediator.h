#pragma once
#ifdef _WIN64
#pragma comment (lib, "SccMediator64.lib")
#else
#pragma comment (lib, "SccMediator86.lib")
#endif
#include "../SccMediatorLib/SccTcpMediator.h"
#include <iostream>

#include <tchar.h>
#include <iomanip>
#include <bitset>

#define STD std::

/**	0 - nothing
*	1 - async
*		results of functional tests
*	2 - sync
*		results of functional tests
*	3 - sync
*		results of functional tests
*		results of inner functions
*	4 - sync
*		results of functional tests
*		results of inner functions
*		recieved words
*/

#define DEBUG_OUTPUT_LEVEL 3
#define ARINC_429_CHANNELS	4
#define ARINC_708_CHANNELS	1


#if DEBUG_OUTPUT_LEVEL > 1
#define SYNCTYPE 
#else
#define SYNCTYPE STD
#endif

#ifdef UNICODE
#define COUT wcout
#define ENDL (_T("\n"))
#else
#define COUT cout
#define ENDL (STD endl)
#endif

struct COUTS {
private:
	std::mutex ioMutex;
public:
	template<typename T>
	COUTS& operator <<(T&&x) {
		std::lock_guard<std::mutex> lock(ioMutex);
		STD COUT << x;
		return *this;
	}
};
static COUTS COUT;

#define CONSOLE_DEBUG_OUTPUT (SYNCTYPE COUT)

class TestMediator
{
	HANDLE _stopEvent;

	STD mutex _ioMutex;

	HWND _consoleWindowHandle;

	bool InitEvents();

	static bool InitLib();

	void PrintMenu() const;

	bool IsActiveConsoleWindow() const;

	static void Send429(HANDLE stopEvent)
	{
		/* set parameters*/
		auto words429Count = 100;
		auto channelNum = 0;

		/* init words*/
		auto words429 = new Word429[words429Count];

		for (auto i = 0; i < words429Count; ++i)
		{
			words429[i].data = i;
			words429[i].time = 500 * (1 + i % 2);
		}

		/* init device */
		auto serialNumber = GetFirstDeviceSN();

		/* setup channel */
		auto resSetOut = SccTcpMediator::Set429OutputChannelParams(serialNumber, channelNum, Arinc429Rate::R100, Arinc429ParityTypeOut::NoChange);
#if DEBUG_OUTPUT_LEVEL > 2
		if (resSetOut < 0)
			CONSOLE_DEBUG_OUTPUT << _T("Set429OutputChannelParams failed with error code: ") << resSetOut << ENDL;
#endif
		// thread
		while (WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0) {
			Sleep(500);
			auto outputBufferMs = static_cast<INT>(SccTcpMediator::Get429OutputBufferMicroseconds(serialNumber, channelNum));
			if (outputBufferMs < 0)
			{
#if DEBUG_OUTPUT_LEVEL > 2
				CONSOLE_DEBUG_OUTPUT << _T("outputBufferMs failed with error: ") << outputBufferMs << ENDL;
#endif
				continue;
			}
			else
			{
				if (outputBufferMs < 300000)
				{
					auto sendRec = SccTcpMediator::Send429WordsRaw(serialNumber, channelNum, words429, words429Count);
#if DEBUG_OUTPUT_LEVEL > 2
					CONSOLE_DEBUG_OUTPUT << _T("Words sent: ") << sendRec << ENDL;
#endif
				}
			}
		}
		// cleanup
		delete[] words429;
		SccTcpMediator::Disconnect();
	}

	static void Listen429(HANDLE stopEvent, UINT offset)
	{
		/* set parameters*/
		auto words429Count = 200;
		auto channelNum = 0;

		auto words429 = new Word429[words429Count];

		/* init device*/
		auto serialNumber = GetFirstDeviceSN();

		/* setup channel */
		auto resSetIn = SccTcpMediator::Set429InputChannelParams(serialNumber, channelNum, Arinc429Rate::R100, Arinc429ParityTypeIn::NoChange);
#if (DEBUG_OUTPUT_LEVEL > 2)
		if (resSetIn < 0)
			CONSOLE_DEBUG_OUTPUT << _T("Set429InputChannelParams failed with error code: ") << resSetIn << ENDL;
#endif
		// thread
		while (WaitForSingleObject(stopEvent, 0) != WAIT_OBJECT_0)
		{
			Sleep(250 + offset / 2 * 25);
			ZeroMemory(words429, words429Count * sizeof(Word429));
			auto resultRec = SccTcpMediator::Receive429WordsRaw(serialNumber, channelNum, words429, words429Count);
			if (resultRec > 0)
			{
#if (DEBUG_OUTPUT_LEVEL > 2)
				CONSOLE_DEBUG_OUTPUT << std::setw(offset) << "Words recieved: " << resultRec << ENDL;
#endif
#if (DEBUG_OUTPUT_LEVEL > 3)
				for (auto i = 0; i < resultRec; ++i)
					CONSOLE_DEBUG_OUTPUT << std::setw(offset) << "Time: " << words429[i].time
					<< " Data: " << std::hex << words429[i].data << std::dec << "\n";
#endif
				continue;
			}
#if (DEBUG_OUTPUT_LEVEL > 2)
			if (resultRec < 0)
				CONSOLE_DEBUG_OUTPUT << std::setw(offset) << "Error Get429WordsAndPrintThread: " << resultRec << ENDL;
#endif
		}
		delete[] words429;
		SccTcpMediator::Disconnect();
	}

	std::vector<HANDLE> _recEvents;
	std::vector<HANDLE> _sendEvents;

	std::vector<std::thread> _threads;

protected:

	static void OnDeviceDetached(ULONG deviceID)
	{
		CONSOLE_DEBUG_OUTPUT << _T("Device removed: ") << deviceID << ENDL;
	}

	static void OnDeviceAttached(ULONG deviceID)
	{
		CONSOLE_DEBUG_OUTPUT << _T("Device attached: ") << deviceID << ENDL;
	}

	static void Ping()
	{

		auto pingMc = SccTcpMediator::Ping();
#if (DEBUG_OUTPUT_LEVEL > 0)
		if (pingMc > 0)
			CONSOLE_DEBUG_OUTPUT << _T("Ping: ") << pingMc << _T(" microseconds.") << ENDL;
		else
			CONSOLE_DEBUG_OUTPUT << _T("Ping failed with error code: ") << pingMc << ENDL;
#endif
	}

	static void GetDllVersion()
	{
		auto resV = SccTcpMediator::GetDllVersion();
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("DLL version") << (resV < 0 ? _T(" failed with error: ") : _T(": ")) << resV << ENDL;
#endif
	}

	static void GetPinConf()
	{
		auto deviceSn = GetFirstDeviceSN();
		auto resV = SccTcpMediator::GetPinConfiguration(deviceSn);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Pin configuration ") << (resV < 0 ? _T(" failed with error: ") : _T(": ")) << resV << ENDL;
#endif
	}

	static INT GetFirstDeviceSN()
	{
		/* set parameters*/
		auto maxDeviceCount = 1;
		auto sn = -1;
		/* init device*/
		auto numbersRaw = new UINT[maxDeviceCount];
		auto devCount = SccTcpMediator::GetDeviceNumsRaw(numbersRaw, maxDeviceCount);
		if (devCount > 0)
			sn = numbersRaw[0];

		if (devCount < 0) {
#if (DEBUG_OUTPUT_LEVEL > 0)
			CONSOLE_DEBUG_OUTPUT << _T("GetDeviceNumsRaw failed wit error: ") << devCount << ENDL;
#endif
			sn = devCount;
		}

		delete[] numbersRaw;
		return sn;

	}

	static void SubscribeToEvents()
	{
		SccTcpMediator::SetDeviceAttachedHandler(OnDeviceAttached);
		SccTcpMediator::SetDeviceDetachedHandler(OnDeviceDetached);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Subscribed to device Attached / Detached events.") << ENDL;
#endif
	}

	static void ResetBuffer()
	{
		/* set parameters*/
		auto channelNum = 0;

		/* init device*/
		auto serialNumber = GetFirstDeviceSN();

		auto retVal = SccTcpMediator::ResetOut429Channel(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Reset OUT 429 Channel. ") << (retVal == NOERROR ? _T("OK\n") : _T("FAILED with error: "));
		if (retVal)
			CONSOLE_DEBUG_OUTPUT << retVal << ENDL;
#endif
		retVal = SccTcpMediator::ResetIn429Channel(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Reset IN  429 Channel. ") << (retVal == NOERROR ? _T("OK\n") : _T("FAILED with error: "));
		if (retVal)
			CONSOLE_DEBUG_OUTPUT << retVal << ENDL;
#endif
	}

	static void ShowChannelsStates()
	{
		/* init device*/
		auto serialNumber = GetFirstDeviceSN();

		auto ch = SccTcpMediator::GetAvalibleChannelsIn(serialNumber);
#if (DEBUG_OUTPUT_LEVEL > 0)
		if (ch < 0 || ch > 31)
			CONSOLE_DEBUG_OUTPUT << _T("Failed to get input channels states! Error code: ") << ch << _T(".") << ENDL;
		else
		{
			std::bitset<5> states(ch);
			for (auto i = 0; i < ARINC_429_CHANNELS;++i)
				CONSOLE_DEBUG_OUTPUT << _T("Channel ¹ ") << i+1 << _T(" 429 INPUT ") << (states[i] == 1 ? _T("is avaliable") : _T("is not avaliable")) << ENDL;
			CONSOLE_DEBUG_OUTPUT << _T("Channel ¹ ") << 1 << _T(" 708 INPUT ") << (states[4] == 1 ? _T("is avaliable") : _T("is not avaliable")) << ENDL;
		}
		ch = SccTcpMediator::GetAvalibleChannelsOut(serialNumber);
		if (ch < 0 || ch > 31)
			CONSOLE_DEBUG_OUTPUT << L"Failed to get output channels states! Error code: " << ch << "." << ENDL;
		else
		{
			std::bitset<5> states2(ch);
			for (auto i = 0; i < ARINC_429_CHANNELS; ++i)
				CONSOLE_DEBUG_OUTPUT << _T("Channel ¹ ") << i+1 << _T(" 429 OUTPUT ") << (states2[i] == 1 ? _T("is avaliable") : _T("is not avaliable")) << ENDL;
			CONSOLE_DEBUG_OUTPUT << _T("Channel ¹ ") << 1 << _T(" 708 OUTPUT ") << (states2[4] == 1 ? _T("is avaliable") : _T("is not avaliable")) << ENDL;
		}
#endif
	}

	static void Release429()
	{
		/* set parameters*/
		auto channelNum = 0;

		/* init device*/
		auto serialNumber = GetFirstDeviceSN();

		auto retC = SccTcpMediator::ReleaseChannel429In(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Release channel 0 429 IN. ") << (retC == TRUE ? _T("OK\n") : _T("FAILED with error: "));
		if (retC != TRUE)
			CONSOLE_DEBUG_OUTPUT << retC << ENDL;
#endif
		retC = SccTcpMediator::ReleaseChannel429Out(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Release channel 0 429 OUT. ") << (retC == TRUE ? _T("OK\n") : _T("FAILED with error: "));
		if (retC != TRUE)
			CONSOLE_DEBUG_OUTPUT << retC << ENDL;
#endif
	}

	static void ConnectForced()
	{
		/* set parameters*/
		auto channelNum = 0;

		/* init device*/
		auto serialNumber = GetFirstDeviceSN();

		auto retC = SccTcpMediator::ConnectForced429In(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Connect Forced to channel 0 429 IN. ") << (retC == TRUE ? _T("OK\n") : _T("FAILED with error: "));
		if (retC != TRUE)
			CONSOLE_DEBUG_OUTPUT << retC << ENDL;
#endif
		retC = SccTcpMediator::ConnectForced429Out(serialNumber, channelNum);
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Connect Forced to channel 0 429 OUT. ") << (retC == TRUE ? _T("OK\n") : _T("FAILED with error: "));
		if (retC != TRUE)
			CONSOLE_DEBUG_OUTPUT << retC << ENDL;
#endif
	}

	void AddSender429()
	{
		_sendEvents.emplace_back(CreateEvent(nullptr, TRUE, FALSE, nullptr));
		_threads.emplace_back(std::thread(Send429, _sendEvents.back()));
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << static_cast<UINT>(_sendEvents.size()) << _T("'th sending 429 thread started.") << ENDL;
#endif
	}

	void RemoveSender()
	{
#if (DEBUG_OUTPUT_LEVEL > 0)
		if (_sendEvents.empty())
		{
			CONSOLE_DEBUG_OUTPUT << _T("No sending 429 threads.") << ENDL;
			return;
		}
#endif
		SetEvent(_sendEvents.back());
		_sendEvents.pop_back();
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Sending 429 thread stopped, running: ") << static_cast<UINT>(_sendEvents.size()) << ENDL;
#endif
	}

	void AddListener429()
	{
		_recEvents.emplace_back(CreateEvent(nullptr, TRUE, FALSE, nullptr));
		_threads.emplace_back(std::thread(Listen429, _recEvents.back(), 20 * _recEvents.size()));
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << static_cast<UINT>(_recEvents.size()) << _T("'th listening thread started.") << ENDL;
#endif
	}

	void RemoveListener429()
	{
#if (DEBUG_OUTPUT_LEVEL > 0)
		if (_recEvents.empty())
		{
			CONSOLE_DEBUG_OUTPUT << _T("No listening 429 threads.") << ENDL;
			return;
		}
#endif
		SetEvent(_recEvents.back());
		_recEvents.pop_back();
#if (DEBUG_OUTPUT_LEVEL > 0)
		CONSOLE_DEBUG_OUTPUT << _T("Listening 429 thread stopped, running: ") << static_cast<UINT>(_recEvents.size()) << ENDL;
#endif
	}

	/*
	SccTcpMediator_API  std::vector<UINT> GetDeviceNums();

	429

	SccTcpMediator_API  INT Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> &words);
	SccTcpMediator_API  INT Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words);
	SccTcpMediator_API UINT Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	SccTcpMediator_API INT Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);

	708

	SccTcpMediator_API  INT Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> &words);
	SccTcpMediator_API  INT Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words);

	SccTcpMediator_API INT ReleaseChannel708In(UINT deviceSerialNum, UINT channelNum);
	SccTcpMediator_API INT ReleaseChannel708Out(UINT deviceSerialNum, UINT channelNum);

	SccTcpMediator_API INT ConnectForced708In(UINT deviceSerialNum, UINT channelNum);
	SccTcpMediator_API INT ConnectForced708Out(UINT deviceSerialNum, UINT channelNum);

	SccTcpMediator_API UINT Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum);
	SccTcpMediator_API UINT Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum);

	SccTcpMediator_API INT Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount);

	SccTcpMediator_API INT ResetOut708Channel(UINT deviceSerialNum, UINT channelNum);
	SccTcpMediator_API INT ResetIn708Channel(UINT deviceSerialNum, UINT channelNum);

	SccTcpMediator_API INT Send708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT count);
	SccTcpMediator_API INT Receive708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT max_count);
	 */
public:
	TestMediator();
	void Start();
	void Stop();
	virtual ~TestMediator();
};

