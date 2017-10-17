#pragma once

#include <array>
#include <map>

#include "IVirtualChannel.h"
#include "CircleBuffer.h"

template <typename T, size_t maxSize>
class IChannelIn : public IVirtualChannel
{
public:
	typedef typename std::array<T, maxSize>::iterator ArrayIterator;
	struct OffsetInfo
	{
		ArrayIterator iterator_;
		UINT ciclesCount_;
		OffsetInfo() = delete;
		OffsetInfo(ArrayIterator iterator, UINT ciclesCount) : iterator_(iterator), ciclesCount_(ciclesCount) {}
		~OffsetInfo() {}
	};
private:

	CircleBuffer<T, maxSize> _circleBuffer;
	std::map<size_t, OffsetInfo> _ownersOffsetsMap;

public:
	// Reset Buffer
	virtual void ResetBuffer()
	{
		std::for_each(_ownersOffsetsMap.begin(), _ownersOffsetsMap.end(),
			[&_circleBuffer = _circleBuffer](auto &mapEntry)
		{
			mapEntry.second.iterator_ = _circleBuffer.GetTail();
			mapEntry.second.ciclesCount_ = _circleBuffer.GetCycle();
		}
		);
	}

	// Return true if ownerId is in map, false if not 
	virtual bool IsReceiving(size_t ownerId)
	{
		return _ownersOffsetsMap.find(ownerId) != _ownersOffsetsMap.end();
	}
	// Add ownerId to the map with OffsetInfo equal to buffer tail and current cycle 
	virtual void AddRecipient(size_t ownerId)
	{
		_ownersOffsetsMap.emplace(std::make_pair(ownerId, OffsetInfo(_circleBuffer.GetTail(), _circleBuffer.GetCycle())));
	}
	// Remove ownerId from the map of recipients
	virtual void RemoveRecipient(size_t ownerId)
	{
		if (IsReceiving(ownerId))
			_ownersOffsetsMap.erase(ownerId);
	}
	// Return offset of ownerId and create an entry with zero offset value if owner does not exist
	ArrayIterator GetOwnersOffset(size_t ownerId) {
		auto res = _ownersOffsetsMap.find(ownerId);
		if (res == _ownersOffsetsMap.end())
			AddRecipient(ownerId);
		return res->second.iterator_;
	}

	virtual INT ReceiveWordsRaw(UINT deviceSerialNum, UINT channelNum, T* words, UINT count, size_t ownerId) = 0;

	virtual INT SynchronizeWithBuffer(T* words, size_t bufferRealSize, size_t bufferMaxSize, size_t ownerId)
	{
		auto it = _ownersOffsetsMap.find(ownerId);
		if (it == _ownersOffsetsMap.end())
			return 0;

		_circleBuffer.Add(words, bufferRealSize);
		// Difference between previous and current cycle numbers 
		auto diff = _circleBuffer.GetCycle() - it->second.ciclesCount_;
		// Defference more than 1 means overwriting data in buffer. 
		if (diff > 1)
			return bufferRealSize;
		UINT countAfterSync = NULL;
		// Write buffer to the words array and return new offset and count of words have beed read from ring buffer
		it->second.iterator_ = _circleBuffer.Write(words, bufferMaxSize, countAfterSync, GetOwnersOffset(ownerId));
		it->second.ciclesCount_ = _circleBuffer.GetCycle();

		return countAfterSync;
	}
	explicit IChannelIn(UINT channelNumber) :IVirtualChannel(channelNumber) {}
	virtual ~IChannelIn() {}
};
