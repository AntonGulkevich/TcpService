#pragma once

#include "Windows.h"
#include <vector>
#include <memory>
#include <bitset>

#include "../../../SccLib/SccStaticLib/SccDeviceLib/SccDeviceLib.h"
#include "../SmallWebProtocols/Message.h"

#include "IChannelIn.h"
#include "IChannelOut.h"

#define MAX_429_CHANNELS 4UL
#define MAX_708_CHANNELS 1UL


#pragma region In Channels

class Arinc708In : public IChannelIn<Word708, MAX_WORD708_COUNT>
{
public:
	INT ReceiveWordsRaw(UINT deviceSerialNum, UINT channelNum, Word708* words, UINT count, size_t ownerId) override;
	ChannelTypeFull GetType() override{ return In708; }
	explicit Arinc708In(UINT channelNumber):IChannelIn(channelNumber) {}
	virtual ~Arinc708In() {}
};

class Arinc429In : public IChannelIn<Word429, MAX_WORD429_COUNT>
{
public:
	INT ReceiveWordsRaw(UINT deviceSerialNum, UINT channelNum, Word429* words, UINT count, size_t ownerId) override;
	ChannelTypeFull GetType() override{ return In429; }
	explicit Arinc429In(UINT channelNumber) :IChannelIn(channelNumber) {}
	virtual ~Arinc429In() {}
};

#pragma endregion 

#pragma region Out Channels

class Arinc429Out : public IChannelOut
{
public:
	ChannelTypeFull GetType() override { return Out429; }
	explicit Arinc429Out(UINT channelNumber) :IChannelOut(channelNumber) {}
	virtual ~Arinc429Out() {};
};

class Arinc708Out : public IChannelOut
{
public:
	ChannelTypeFull GetType() override { return Out708; }
	explicit Arinc708Out(UINT channelNumber) :IChannelOut(channelNumber) {}
	virtual ~Arinc708Out() {};

};
#pragma endregion 

#pragma region VirtualDevice
class VirtualDevice
{
	// Serial number of real device
	UINT _deviceSerialNumber;

	// In channels
	std::vector<std::unique_ptr<Arinc429In>>  _inChannels429;
	std::vector<std::unique_ptr<Arinc708In>>  _inChannels708;

	// Out channels
	std::vector<std::unique_ptr<Arinc429Out>> _outChannels429;
	std::vector<std::unique_ptr<Arinc708Out>> _outChannels708;

	// Return true is channelnumber is in proper range due to its channels type
	static bool IsChannelNumberCorrect(ChannelTypeFull channelType, UINT channelnumber);

	// Fill vectors of channels
	void InitializeChannels();

	// Clear vectors of channels 
	void ClearChannels();

	// Helpers
	void RemoveRecipient429(size_t ownerId);
	void RemoveRecipient708(size_t ownerId);

	// Clear info about input channels used by client
	void RemoveRecipient(size_t ownerId);

public:

	// Return count of released chanels owned by Client 
	UINT RemoveClient(size_t ownerId);

	INT Receive708WordsRaw(size_t ownerId, UINT channelNum, Word708* words, UINT count);
	INT Receive429WordsRaw(size_t ownerId, UINT channelNum, Word429* words, UINT count);

	// Return TRUE on success, error code on failure
	BOOL SetOwner(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID);

	// Return TRUE on success, error code on failure
	BOOL IsAvaliableFor(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID);

	// Return TRUE on success, error code on failure
	BOOL IsPwnedBy(ChannelTypeFull channelType, UINT channelNumber, size_t ownerID);

	// Return TRUE on success, error code on failure
	BOOL ResetBuffer(ChannelTypeFull channelType, UINT channelNumber);

	// Return UINT, bits in one mean is channel avaliable or not
	// 1 - is avaliable, 0 - is aready on use
	// first 4 bits bits are 1-4 numbers of 429 channels
	// 5th bit is 708 channel
	UINT GetChannelsStateIn(size_t ownerID);
	UINT GetChannelsStateOut(size_t ownerID);

	// Ctors 
	VirtualDevice() = delete;
	explicit VirtualDevice(UINT serialNumber);
	virtual ~VirtualDevice();
};

#pragma endregion


