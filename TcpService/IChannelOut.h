#pragma once
#include "IVirtualChannel.h"
class IChannelOut : public IVirtualChannel
{
public:
	explicit IChannelOut(UINT channelNumber) : IVirtualChannel(channelNumber) {}
	virtual ~IChannelOut() {}
};

