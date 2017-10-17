#include "VirtualDevice.h"

void VirtualDevice::InitializeChannels()
{
	for (UINT i = 0; i < MAX_429_CHANNELS; ++i)
	{
		_inChannels429.emplace_back(std::make_unique<Arinc429In>(i));
		_outChannels429.emplace_back(std::make_unique<Arinc429Out>(i));
	}

	for (UINT i = 0; i < MAX_708_CHANNELS; ++i)
	{
		_inChannels708.emplace_back(std::make_unique<Arinc708In>(i));
		_outChannels708.emplace_back(std::make_unique<Arinc708Out>(i));
	}
}

void VirtualDevice::ClearChannels()
{
	_inChannels429.clear();
	_inChannels708.clear();
	_outChannels429.clear();
	_outChannels708.clear();
}

void VirtualDevice::RemoveRecipient429(size_t ownerId)
{
	for (auto && channel : _inChannels429)
		channel->RemoveRecipient(ownerId);
}

void VirtualDevice::RemoveRecipient708(size_t ownerId)
{
	for (auto && channel : _inChannels708)
		channel->RemoveRecipient(ownerId);
}

UINT VirtualDevice::RemoveClient(size_t ownerId)
{
	RemoveRecipient(ownerId);

	UINT count = 0;
	for (auto&& channel : _inChannels429)
	{
		if (channel->GetOwnerID() == ownerId)
		{
			channel->SetOwnerID(NULL);
			++count;
		}
	}

	for (auto&& channel : _inChannels708)
	{
		if (channel->GetOwnerID() == ownerId)
		{
			channel->SetOwnerID(NULL);
			++count;
		}
	}
	for (auto&& channel : _outChannels429)
	{
		if (channel->GetOwnerID() == ownerId)
		{
			channel->SetOwnerID(NULL);
			++count;
		}
	}
	for (auto&& channel : _outChannels708)
	{
		if (channel->GetOwnerID() == ownerId)
		{
			channel->SetOwnerID(NULL);
			++count;
		}
	}
	return count;
}

bool VirtualDevice::IsChannelNumberCorrect(ChannelTypeFull channelType, UINT channelnumber)
{
	switch (channelType)
	{
	case In429: return channelnumber < MAX_429_CHANNELS;
	case Out429: return channelnumber < MAX_429_CHANNELS;
	case In708: return channelnumber < MAX_708_CHANNELS;
	case Out708: return channelnumber < MAX_708_CHANNELS;
	default: return false;
	}
}

void VirtualDevice::RemoveRecipient(size_t ownerId)
{
	RemoveRecipient429(ownerId);
	RemoveRecipient708(ownerId);
}

INT VirtualDevice::Receive708WordsRaw(size_t ownerId, UINT channelNum, Word708* words, UINT count)
{
	if (!IsChannelNumberCorrect(Out708, channelNum))
		return INVALID_CHANNEL_NUMBER;
	return _inChannels708.at(channelNum)->ReceiveWordsRaw(_deviceSerialNumber, channelNum, words, count, ownerId);
}

INT VirtualDevice::Receive429WordsRaw(size_t ownerId, UINT channelNum, Word429* words, UINT count)
{
	if (!IsChannelNumberCorrect(Out429, channelNum))
		return INVALID_CHANNEL_NUMBER;
	return _inChannels429.at(channelNum)->ReceiveWordsRaw(_deviceSerialNumber, channelNum, words, count, ownerId);
}

BOOL VirtualDevice::SetOwner(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID)
{
	if (IsChannelNumberCorrect(channelType, channelNumber))
	{
		switch (channelType)
		{
		case In429:
		{
			_inChannels429.at(channelNumber)->SetOwnerID(ownerID);
			break;
		}
		case Out429:
		{
			_outChannels429.at(channelNumber)->SetOwnerID(ownerID);
			break;
		}
		case In708:
		{
			_inChannels708.at(channelNumber)->SetOwnerID(ownerID);
			break;
		}
		case Out708:
		{
			_outChannels708.at(channelNumber)->SetOwnerID(ownerID);
			break;
		}
		default:
			return INVALID_CHANNEL_TYPE;
		}
		return TRUE;
	}
	return INVALID_CHANNEL_NUMBER;
}

BOOL VirtualDevice::IsAvaliableFor(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID)
{
	if (IsChannelNumberCorrect(channelType, channelNumber))
	{
		switch (channelType)
		{
		case In429: return static_cast<BOOL>(_inChannels429.at(channelNumber)->IsAvaliableFor(ownerID));
		case Out429: return static_cast<BOOL>(_outChannels429.at(channelNumber)->IsAvaliableFor(ownerID));
		case In708: return static_cast<BOOL>(_inChannels708.at(channelNumber)->IsAvaliableFor(ownerID));
		case Out708: return static_cast<BOOL>(_outChannels708.at(channelNumber)->IsAvaliableFor(ownerID));
		default: return INVALID_CHANNEL_TYPE;
		}
	}
	return INVALID_CHANNEL_NUMBER;
}

BOOL VirtualDevice::IsPwnedBy(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID)
{
	if (IsChannelNumberCorrect(channelType, channelNumber))
	{
		switch (channelType)
		{
		case In429: return static_cast<BOOL>(_inChannels429.at(channelNumber)->IsPwnedBy(ownerID));
		case Out429: return static_cast<BOOL>(_outChannels429.at(channelNumber)->IsPwnedBy(ownerID));
		case In708: return static_cast<BOOL>(_inChannels708.at(channelNumber)->IsPwnedBy(ownerID));
		case Out708: return static_cast<BOOL>(_outChannels708.at(channelNumber)->IsPwnedBy(ownerID));
		default: return INVALID_CHANNEL_TYPE;
		}
	}
	return INVALID_CHANNEL_NUMBER;
}

UINT VirtualDevice::GetChannelsStateIn(size_t ownerID)
{
	std::bitset<size_t(MAX_429_CHANNELS + MAX_708_CHANNELS)> channelStatesImpl;
	for (auto i = 0; i < _inChannels429.size(); ++i)
	{
		if (IsAvaliableFor(In429, i, ownerID))
			channelStatesImpl.set(i);
	}
	for (auto i = 0; i < _inChannels708.size(); ++i)
	{
		if (IsAvaliableFor(In708, i, ownerID))
			channelStatesImpl.set(i + MAX_429_CHANNELS);
	}
	return channelStatesImpl.to_ulong();
}

UINT VirtualDevice::GetChannelsStateOut(size_t ownerID)
{
	std::bitset<5> channelStatesImpl;
	for (auto i = 0; i < _outChannels429.size(); ++i)
		if (IsAvaliableFor(Out429, i, ownerID))
			channelStatesImpl.set(i);
	for (auto i = 0; i < _outChannels708.size(); ++i)
		if (IsAvaliableFor(Out708, i, ownerID))
			channelStatesImpl.set(i + MAX_429_CHANNELS);
	return channelStatesImpl.to_ulong();
}

VirtualDevice::VirtualDevice(UINT serialNumber) : _deviceSerialNumber(serialNumber)
{
	InitializeChannels();
}

VirtualDevice::~VirtualDevice()
{
	ClearChannels();
}

INT Arinc429In::ReceiveWordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, UINT count, size_t ownerId)
{
	if (!IsReceiving(ownerId))
		AddRecipient(ownerId);

	auto countFromLib = SccLibInterop::Receive429WordsRaw(deviceSerialNum, channelNum, words, count);
	if (countFromLib >= 0)
		return SynchronizeWithBuffer(words, countFromLib, count, ownerId);
	return countFromLib;
}

INT Arinc708In::ReceiveWordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT count, size_t ownerId)
{
	if (!IsReceiving(ownerId))
		AddRecipient(ownerId);

	auto countFromLib = SccLibInterop::Receive708WordsRaw(deviceSerialNum, channelNum, words, count);

	if (countFromLib >= 0)
		return SynchronizeWithBuffer(words, countFromLib, count, ownerId);

	return countFromLib;
}

BOOL VirtualDevice::ResetBuffer(ChannelTypeFull channelType, UINT channelNumber)
{
	if (!IsChannelNumberCorrect(channelType, channelNumber))
		return INVALID_CHANNEL_NUMBER;

	switch (channelType)
	{
	case In429:
	{
		_inChannels429.at(channelNumber)->ResetBuffer();
		break;
	}
	case In708:
	{
		_inChannels708.at(channelNumber)->ResetBuffer();
		break;
	}
	default:
		return INVALID_CHANNEL_TYPE;
	}
	return TRUE;
}
