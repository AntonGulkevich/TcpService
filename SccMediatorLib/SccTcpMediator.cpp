#include  "SccTcpMediator.h"

namespace SccTcpMediator
{
	LPSTMDATA sccTcpMediator = nullptr;

	// return on success: TRUE
	// return on failure: FALSE
	// OUT parameter: errorCode
	// OUT parameter: errorHint
	BOOL checkState(UINT &errorCode, CHAR *errorHint)
	{
		if (sccTcpMediator == nullptr) {
			errorCode = 100;
			auto errorHint_ = "Successful STMStartup not yet performed.";
			strcpy_s(errorHint, strlen(errorHint_), errorHint_);
			return FALSE;
		}
		return TRUE;
	}

	int Set429InputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeIn parityType)
	{
		return sccTcpMediator->Set429InputChannelParams(deviceSerialNum, channelNum, rate, parityType);
	}

	INT Set429OutputChannelParams(UINT deviceSerialNum, UINT channelNum, Arinc429Rate rate, Arinc429ParityTypeOut parityType)
	{
		return sccTcpMediator->Set429OutputChannelParams(deviceSerialNum, channelNum, rate, parityType);
	}

	UINT Get429OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->Get429OutputBufferWordsCount(deviceSerialNum, channelNum);
	}

	UINT Get708OutputBufferWordsCount(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->Get708OutputBufferWordsCount(deviceSerialNum, channelNum);
	}

	UINT Get429OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->Get429OutputBufferMicroseconds(deviceSerialNum, channelNum);
	}

	UINT Get708OutputBufferMicroseconds(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->Get708OutputBufferMicroseconds(deviceSerialNum, channelNum);
	}

	INT Set429InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
	{
		return sccTcpMediator->Set429InputBufferLength(deviceSerialNum, channelNum, wordsCount);
	}

	INT Set708InputBufferLength(UINT deviceSerialNum, UINT channelNum, UINT wordsCount)
	{
		return sccTcpMediator->Set708InputBufferLength(deviceSerialNum, channelNum, wordsCount);
	}

	INT ResetOut429Channel(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ResetOut429Channel(deviceSerialNum, channelNum);
	}

	INT ResetOut708Channel(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ResetOut708Channel(deviceSerialNum, channelNum);
	}

	INT ResetIn429Channel(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ResetIn429Channel(deviceSerialNum, channelNum);
	}

	INT ResetIn708Channel(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ResetIn708Channel(deviceSerialNum, channelNum);
	}

	void SetDeviceAttachedHandler(DeviceListChanged handler)
	{
		sccTcpMediator->SetDeviceAttachedHandler(handler);
	}

	void SetDeviceDetachedHandler(DeviceListChanged handler)
	{
		sccTcpMediator->SetDeviceDetachedHandler(handler);
	}

	INT GetDeviceNumsRaw(UINT* nums, UINT max_size)
	{
		return sccTcpMediator->GetDeviceNumsRaw(nums, max_size);
	}

	INT Send429WordsRaw(UINT deviceSerialNum, UINT channelNum, const Word429* words, UINT count)
	{
		return sccTcpMediator->SendWords(deviceSerialNum, channelNum, words, count);
	}

	INT Receive429WordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, UINT count)
	{
		return sccTcpMediator->RecWords(deviceSerialNum, channelNum, words, count);
	}

	INT Send708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT count)
	{
		return sccTcpMediator->SendWords(deviceSerialNum, channelNum, words, count);
	}

	INT Receive708WordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT max_count)
	{
		return sccTcpMediator->RecWords(deviceSerialNum, channelNum, words, max_count);
	}

	INT GetAvalibleChannelsIn(UINT deviceSerialNum)
	{
		return sccTcpMediator->GetAvalibleChannelsIn(deviceSerialNum);
	}

	INT GetAvalibleChannelsOut(UINT deviceSerialNum)
	{
		return sccTcpMediator->GetAvalibleChannelsOut(deviceSerialNum);
	}

	INT ReleaseChannel429In(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ReleaseChannel429In(deviceSerialNum, channelNum);
	}

	INT ReleaseChannel429Out(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ReleaseChannel429Out(deviceSerialNum, channelNum);
	}

	INT ReleaseChannel708In(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ReleaseChannel708In(deviceSerialNum, channelNum);
	}

	INT ReleaseChannel708Out(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ReleaseChannel708Out(deviceSerialNum, channelNum);
	}

	INT ConnectForced429In(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ConnectForced429In(deviceSerialNum, channelNum);
	}

	INT ConnectForced429Out(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ConnectForced429Out(deviceSerialNum, channelNum);
	}

	INT ConnectForced708In(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ConnectForced708In(deviceSerialNum, channelNum);
	}

	INT ConnectForced708Out(UINT deviceSerialNum, UINT channelNum)
	{
		return sccTcpMediator->ConnectForced708Out(deviceSerialNum, channelNum);
	}

	INT STMStartup(LPTSTR hostNameTCP, USHORT port, LPWSTR hostNameUDP)
	{
		if (hostNameUDP == nullptr)
			hostNameUDP = DEFAULT_UDP_ADDRESS;
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			WSACleanup();
			return GetLastError();
		}
		if (hostNameTCP == nullptr)
		{
			hostNameTCP = new WCHAR[46];
			char ac[NI_MAXHOST];
			if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
				return GetLastError();
			ADDRINFO hints;
			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			ADDRINFO* result = nullptr;
			auto dwRetval = getaddrinfo(ac, nullptr, &hints, &result);
			if (dwRetval != 0)
				return GetLastError();
			DWORD ipbufferlength = 46;
			auto sockaddr_ip = static_cast<LPSOCKADDR>(result->ai_addr);
			WSAAddressToString(sockaddr_ip, static_cast<DWORD>(result->ai_addrlen), nullptr, hostNameTCP, &ipbufferlength);
		}
		// automatically freed at the end of the scope
		sccTcpMediator = &SccMediatorSingleton::Instance();
		sccTcpMediator->SetServerInfo(hostNameTCP, port, hostNameUDP);
		auto err = sccTcpMediator->Start();
		if (err)
		{
			// failed to start mediator services 
			sccTcpMediator = nullptr;
			return err;
		}
		return 0;
	}

	INT STMStartupLocal()
	{
		return STMStartup(nullptr, DEFAULT_PORT, DEFAULT_UDP_ADDRESS);
	}

	INT Ping()
	{
		return sccTcpMediator->Ping();
	}

	void Disconnect()
	{
		sccTcpMediator->DisconnectCurrentClientFromServer();
	}

	INT GetDeviceCount()
	{
		return sccTcpMediator->GetDeviceCount();
	}

	std::vector<UINT> GetDeviceNums()
	{
		return sccTcpMediator->GetDeviceNums();
	}

	INT Send429Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word429> &words)
	{
		return sccTcpMediator->Send429Words(deviceSerialNum, channelNum, words);
	}

	INT Receive429Words(UINT deviceSerialNum, UINT channelNum, std::list<Word429>& words)
	{
		return sccTcpMediator->Receive429Words(deviceSerialNum, channelNum, words);
	}

	INT Send708Words(UINT deviceSerialNum, UINT channelNum, const std::list<Word708> &words)
	{
		return sccTcpMediator->Send708Words(deviceSerialNum, channelNum, words);
	}

	INT Receive708Words(UINT deviceSerialNum, UINT channelNum, std::list<Word708>& words)
	{
		return sccTcpMediator->Receive708Words(deviceSerialNum, channelNum, words);
	}

	bool IsOnline()
	{
		return sccTcpMediator->IsOnline();
	}

	INT GetPinConfiguration(UINT deviceSerialNum)
	{
		return sccTcpMediator->GetPinConfiguration(deviceSerialNum);
	}

	INT GetDllVersion()
	{
		return sccTcpMediator->GetDllVersion();
	}
}
