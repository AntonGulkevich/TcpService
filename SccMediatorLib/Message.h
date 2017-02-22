#pragma once
#include <Windows.h>
#include <vector>

#define HEADER_SIZE 8

class Message
{
	std::vector<BYTE> vec_bytes_;
	void Add(PBYTE data, UINT dataSize);
	void UpdateSize();
public:
	explicit Message(UINT command);
	Message();
	virtual ~Message();

	BOOL Isempty() const;

	UINT GetBytesCount() const;

	PBYTE GetRawData();

	// stun for the template tail
	void Add() const;

	template <typename T>
	void Add(T const& value);

	template <typename Head, typename ... Tail>
	void Add(Head const & head, Tail const & ...tail);

	template <typename T>
	static T ReadFromBytes(PNZCH bytes, UINT offset);

#pragma region Byte Converter Static Functions

	static void AddUintToVector(UINT var, std::vector<BYTE>& vector);

	static void InsertUintToVector(UINT var, std::vector<BYTE>& vector, UINT where);

	static void InsertUintToByteArray(UINT var, PBYTE array, UINT offset);

	static UINT ReadUint(PBYTE ByteArray, UINT Offset);
	
#pragma endregion 
};
template <typename T>
void Message::Add(T const & value)
{
	const void * pData = &value;
	auto src = PBYTE(pData);
	vec_bytes_.insert(vec_bytes_.end(), src, src + sizeof(T));
	UpdateSize();
	//for (auto i = 0; i < sizeof(T); ++i)
	//	vec_bytes_.push_back(static_cast<BYTE>((value >> i * 8) & 0xFF));
}

template <typename Head, typename ... Tail>
void Message::Add(Head const& head, Tail const&... tail)
{
	Add(head);
	if (sizeof...(tail) > 0)
		Add(tail...);
}

template <typename T>
T Message::ReadFromBytes(PNZCH bytes, UINT offset)
{
	//UINT result;
	//memcpy_s(&result, sizeof(UINT), ByteArray + Offset, sizeof(UINT));
	//return result;
	T result;
	memcpy_s(&result, sizeof(T), bytes + offset, sizeof(T));
	return result;
}

