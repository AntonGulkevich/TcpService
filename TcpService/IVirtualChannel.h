#pragma once

enum ChannelTypeFull
{
	In429,
	Out429,

	In708,
	Out708,

	Ignored
};

class IVirtualChannel
{
	UINT _channelNumber;
	size_t _ownerId;
public:
	virtual ChannelTypeFull GetType() = 0;

	void SetOwnerID(size_t ownerId) { _ownerId = ownerId; }

	size_t GetOwnerID() const { return _ownerId; }
	UINT GetChannelNumber() const { return _channelNumber; }

	explicit IVirtualChannel(UINT channelNumber) : _channelNumber(channelNumber), _ownerId(NULL) {}
	virtual ~IVirtualChannel() {}

	bool IsAvaliableFor(size_t ownerId)
	{
		if (_ownerId == NULL)
			_ownerId = ownerId;
		return _ownerId == ownerId;
	}

	bool IsPwnedBy(size_t ownerId) const { return _ownerId == ownerId; }
};

