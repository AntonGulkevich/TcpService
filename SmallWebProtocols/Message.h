#pragma once
#include <vector>
#include <Windows.h>
#define MAX_BUFFER_LEN (0x1000 * 30)
#define HEADER_LENGTH (3 * sizeof(UINT))
#define MAX_WORD429_COUNT ((MAX_BUFFER_LEN/sizeof(Word429)) - HEADER_LENGTH)
#define MAX_WORD708_COUNT ((MAX_BUFFER_LEN/sizeof(Word708)) - HEADER_LENGTH)

enum class CommandHeader
{
#pragma region Maintenance
	Ping,
	Disconnect,
	Attached,
	Detached,
#pragma endregion 

#pragma region Channel Manager
	GetAvalibleChannelsIn,
	GetAvalibleChannelsOut,

	ReleaseChannel429In,
	ReleaseChannel429Out,
	ReleaseChannel708In,
	ReleaseChannel708Out,

	ConnectForced429In,
	ConnectForced429Out,
	ConnectForced708In,
	ConnectForced708Out,
#pragma endregion 

#pragma region SccLib
	GetDeviceCount = 0x100,
	GetDeviceNumsRaw,

	Send429WordsRaw,
	Send708WordsRaw,

	Receive429WordsRaw,
	Receive708WordsRaw,

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

	GetPinConfiguration,
	GetDllVersion

#pragma endregion 

};

class Message
{
	std::vector<CHAR> _rawDataVector;
	Message() = delete;
	void UpdateSize()
	{
		auto size = static_cast<UINT>(_rawDataVector.size());
		for (auto i = 0; i < sizeof(UINT); ++i)
			_rawDataVector[i] = reinterpret_cast<PCHAR>(&size)[i];
	}
public:

	explicit Message(const CommandHeader &command)
	{
		_rawDataVector = { 4, 0, 0, 0 };
		Append(command);
	}

	virtual ~Message() { }

	static void Append() {}

	void Append(PCHAR data, size_t size)
	{
		_rawDataVector.insert(_rawDataVector.end(), data, data + size);
	}

	template<typename T>
	void Append(T const & var) {
		Append(PCHAR(&var), sizeof(T));
	}

	template <typename Head, typename ... Tail>
	void Append(Head const& head, Tail const&... tail)
	{
		Append(head);
		Append(tail...);
	}

	BOOL Isempty() const _NOEXCEPT { return _rawDataVector.empty(); }
	UINT GetBytesCount() const _NOEXCEPT { return UINT(_rawDataVector.size()); }
	// Return address of the first element
	PCNZCH GetRawData() _NOEXCEPT
	{
		UpdateSize();
		const char * cptr = _rawDataVector.data();
		return cptr;
	}
	template <typename T>
	static T ReadPODFromBytes(PNZCH bytes, size_t offset = 0)
	{
		T result;
		memcpy_s(&result, sizeof(T), bytes + offset, sizeof(T));
		return result;
	}
};
