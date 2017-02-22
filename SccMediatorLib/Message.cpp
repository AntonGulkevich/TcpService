#include "Message.h"

BOOL Message::Isempty() const
{
	return vec_bytes_.empty();
}

UINT Message::GetBytesCount() const
{
	return vec_bytes_.size();
}
//uint message size in bytes

PBYTE Message::GetRawData()
{
	//auto size = vec_bytes_.size();
	//if (packetData != nullptr)
	//	delete[] packetData;
	//packetData = new BYTE[size];
	//return memcpy_s(packetData, size, vec_bytes_.data(), size) == NO_ERROR ? packetData : nullptr;
	return vec_bytes_.data();
}

void Message::Add() const
{
}

void Message::Add(PBYTE data, UINT dataSize)
{
	vec_bytes_.insert(vec_bytes_.end(), data, data + dataSize);
	UpdateSize();
}

void Message::UpdateSize()
{
	auto size = vec_bytes_.size();
	vec_bytes_.erase(vec_bytes_.begin(), vec_bytes_.begin() + sizeof(UINT));
	const void * pData = &(size);
	auto src = PBYTE(pData);
	vec_bytes_.insert(vec_bytes_.begin(), src, src + sizeof(UINT));
}

Message::Message(UINT command)
{
	Add(sizeof(UINT), command);
}

Message::Message()
{
	Add(0);
}

Message::~Message()
{
	vec_bytes_.clear();
}

void Message::AddUintToVector(UINT var, std::vector<BYTE>& vector)
{
	for (auto i = 0; i < sizeof(UINT); ++i)
		vector.push_back(static_cast<BYTE>((var >> i * 8) & 0xFF));
}

void Message::InsertUintToVector(UINT var, std::vector<BYTE>& vector, UINT where)
{
	for (auto i = 0; i < sizeof(UINT); ++i)
		vector.insert(vector.begin() + i, static_cast<BYTE>((var >> i * 8) & 0xFF));
}

void Message::InsertUintToByteArray(UINT var, PBYTE array, UINT offset)
{
	for (auto i = offset; i < offset + sizeof(UINT); ++i)
		array[i] = static_cast<BYTE>((var >> i * 8) & 0xFF);
}

UINT Message::ReadUint(PBYTE ByteArray, UINT Offset)
{
	UINT result;
	memcpy_s(&result, sizeof(UINT), ByteArray + Offset, sizeof(UINT));
	return result;
}


