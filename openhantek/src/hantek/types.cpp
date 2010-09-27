////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/types.cpp
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010  Oliver Haag
//  oliver.haag@gmail.com
//
//  This program is free software: you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////


#include <cstring>


#include "hantek/types.h"


namespace Hantek {
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetFilter
	/// \brief Sets the data array to the default values.
	CommandSetFilter::CommandSetFilter() : Helper::DataArray<unsigned char>(8) {
		this->init();
	}
	
	/// \brief Sets the FilterByte to the given value.
	/// \param channel1 true if channel 1 is filtered.
	/// \param channel2 true if channel 2 is filtered.
	/// \param trigger true if trigger is filtered.
	CommandSetFilter::CommandSetFilter(bool channel1, bool channel2, bool trigger) : Helper::DataArray<unsigned char>(8) {
		this->init();
		
		this->setChannel(0, channel1);
		this->setChannel(1, channel2);
		this->setTrigger(trigger);
	}
	
	/// \brief Gets the filtering state of one channel.
	/// \param channel The channel whose filtering state should be returned.
	/// \return The filtering state of the channel.
	bool CommandSetFilter::getChannel(unsigned int channel) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		if(channel == 0)
			return filterBits->channel1 == 1;
		else
			return filterBits->channel2 == 1;
	}
	
	/// \brief Enables/disables filtering of one channel.
	/// \param channel The channel that should be set.
	/// \param filtered true if the channel should be filtered.
	void CommandSetFilter::setChannel(unsigned int channel, bool filtered) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		if(channel == 0)
			filterBits->channel1 = filtered ? 1 : 0;
		else
			filterBits->channel2 = filtered ? 1 : 0;
	}
	
	/// \brief Gets the filtering state for the trigger.
	/// \return The filtering state of the trigger.
	bool CommandSetFilter::getTrigger() {
		return ((FilterBits *) &(this->array[2]))->trigger == 1;
	}
	
	/// \brief Enables/disables filtering for the trigger.
	/// \param filtered true if the trigger should be filtered.
	void CommandSetFilter::setTrigger(bool filtered) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		
		filterBits->trigger = filtered ? 1 : 0;
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetFilter::init() {
		this->array[0] = COMMAND_SETFILTER;
		this->array[1] = 0x0f;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetTriggerAndSamplerate
	/// \brief Sets the data array to the default values.
	CommandSetTriggerAndSamplerate::CommandSetTriggerAndSamplerate() : Helper::DataArray<unsigned char>(12) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param samplerateSlow The SamplerateSlow value.
	/// \param triggerPosition The trigger position value.
	/// \param triggerSource The trigger source id (Tsr1).
	/// \param bufferSize The buffer size id (Tsr1).
	/// \param samplerateFast The samplerateFast value (Tsr1).
	/// \param usedChannels The enabled channels (Tsr2).
	/// \param fastRate The fastRate state (Tsr2).
	/// \param triggerSlope The triggerSlope value (Tsr2).
	CommandSetTriggerAndSamplerate::CommandSetTriggerAndSamplerate(unsigned short int samplerateSlow, unsigned long int triggerPosition, unsigned char triggerSource, unsigned char bufferSize, unsigned char samplerateFast, unsigned char usedChannels, bool fastRate, unsigned char triggerSlope) : Helper::DataArray<unsigned char>(12) {
		this->init();
		
		this->setTriggerSource(triggerSource);
		this->setBufferSize(bufferSize);
		this->setSamplerateFast(samplerateFast);
		this->setUsedChannels(usedChannels);
		this->setFastRate(fastRate);
		this->setTriggerSlope(triggerSlope);
		this->setSamplerateSlow(samplerateSlow);
		this->setTriggerPosition(triggerPosition);
	}
	
	/// \brief Get the triggerSource value in Tsr1Bits.
	/// \return The triggerSource value.
	unsigned char CommandSetTriggerAndSamplerate::getTriggerSource() {
		return ((Tsr1Bits *) &(this->array[2]))->triggerSource;
	}
	
	/// \brief Set the triggerSource in Tsr1Bits to the given value.
	/// \param value The new triggerSource value.
	void CommandSetTriggerAndSamplerate::setTriggerSource(unsigned char value) {
		((Tsr1Bits *) &(this->array[2]))->triggerSource = value;
	}
	
	/// \brief Get the bufferSize value in Tsr1Bits.
	/// \return The #BufferSizeId value.
	unsigned char CommandSetTriggerAndSamplerate::getBufferSize() {
		return ((Tsr1Bits *) &(this->array[2]))->bufferSize;
	}
	
	/// \brief Set the bufferSize in Tsr1Bits to the given value.
	/// \param value The new #BufferSizeId value.
	void CommandSetTriggerAndSamplerate::setBufferSize(unsigned char value) {
		((Tsr1Bits *) &(this->array[2]))->bufferSize = value;
	}
	
	/// \brief Get the samplerateFast value in Tsr1Bits.
	/// \return The samplerateFast value.
	unsigned char CommandSetTriggerAndSamplerate::getSamplerateFast() {
		return ((Tsr1Bits *) &(this->array[2]))->samplerateFast;
	}
	
	/// \brief Set the samplerateFast in Tsr1Bits to the given value.
	/// \param value The new samplerateFast value.
	void CommandSetTriggerAndSamplerate::setSamplerateFast(unsigned char value) {
		((Tsr1Bits *) &(this->array[2]))->samplerateFast = value;
	}
	
	/// \brief Get the usedChannels value in Tsr2Bits.
	/// \return The usedChannels value.
	unsigned char CommandSetTriggerAndSamplerate::getUsedChannels() {
		return ((Tsr2Bits *) &(this->array[3]))->usedChannels;
	}
	
	/// \brief Set the usedChannels in Tsr2Bits to the given value.
	/// \param value The new usedChannels value.
	void CommandSetTriggerAndSamplerate::setUsedChannels(unsigned char value) {
		((Tsr2Bits *) &(this->array[3]))->usedChannels = value;
	}
	
	/// \brief Get the fastRate state in Tsr2Bits.
	/// \return The fastRate state.
	bool CommandSetTriggerAndSamplerate::getFastRate() {
		return ((Tsr2Bits *) &(this->array[3]))->fastRate == 1;
	}
	
	/// \brief Set the fastRate in Tsr2Bits to the given state.
	/// \param fastRate The new fastRate state.
	void CommandSetTriggerAndSamplerate::setFastRate(bool fastRate) {
		((Tsr2Bits *) &(this->array[3]))->fastRate = fastRate ? 1 : 0;
	}
	
	/// \brief Get the triggerSlope value in Tsr2Bits.
	/// \return The triggerSlope value.
	unsigned char CommandSetTriggerAndSamplerate::getTriggerSlope() {
		return ((Tsr2Bits *) &(this->array[3]))->triggerSlope;
	}
	
	/// \brief Set the triggerSlope in Tsr2Bits to the given value.
	/// \param slope The new triggerSlope value.
	void CommandSetTriggerAndSamplerate::setTriggerSlope(unsigned char slope) {
		((Tsr2Bits *) &(this->array[3]))->triggerSlope = slope;
	}
	
	/// \brief Get the SamplerateSlow value.
	/// \return The SamplerateSlow value.
	unsigned short int CommandSetTriggerAndSamplerate::getSamplerateSlow() {
		return (unsigned short int) this->array[4] | ((unsigned short int) this->array[5] << 8);
	}
	
	/// \brief Set the SamplerateSlow to the given value.
	/// \param samplerate The new SamplerateSlow value.
	void CommandSetTriggerAndSamplerate::setSamplerateSlow(unsigned short int samplerate) {
		this->array[4] = (unsigned char) samplerate;
		this->array[5] = (unsigned char) (samplerate >> 8);
	}
	
	/// \brief Get the TriggerPosition value.
	/// \return The horizontal trigger position.
	unsigned long int CommandSetTriggerAndSamplerate::getTriggerPosition() {
		return (unsigned long int) this->array[6] | ((unsigned long int) this->array[7] << 8) | ((unsigned long int) this->array[10] << 16);
	}
	
	/// \brief Set the TriggerPosition to the given value.
	/// \param position The new horizontal trigger position.
	void CommandSetTriggerAndSamplerate::setTriggerPosition(unsigned long int position) {
		this->array[6] = (unsigned char) position;
		this->array[7] = (unsigned char) (position >> 8);
		this->array[10] = (unsigned char) (position >> 16);
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetTriggerAndSamplerate::init() {
		this->array[0] = COMMAND_SETTRIGGERANDSAMPLERATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandForceTrigger
	/// \brief Sets the data array to needed values.
	CommandForceTrigger::CommandForceTrigger() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_FORCETRIGGER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandCaptureStart
	/// \brief Sets the data array to needed values.
	CommandCaptureStart::CommandCaptureStart() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_STARTSAMPLING;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandTriggerEnabled
	/// \brief Sets the data array to needed values.
	CommandTriggerEnabled::CommandTriggerEnabled() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_ENABLETRIGGER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandGetData
	/// \brief Sets the data array to needed values.
	CommandGetData::CommandGetData() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_GETDATA;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandGetCaptureState
	/// \brief Sets the data array to needed values.
	CommandGetCaptureState::CommandGetCaptureState() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_GETCAPTURESTATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ResponseGetCaptureState
	/// \brief Initializes the array.
	ResponseGetCaptureState::ResponseGetCaptureState() : Helper::DataArray<unsigned char>(512) {
	}
	
	/// \brief Gets the capture state.
	/// \return The CaptureState of the oscilloscope.
	CaptureState ResponseGetCaptureState::getCaptureState() {
		return (CaptureState) this->array[0];
	}
	
	/// \brief Gets the trigger point.
	/// \return The trigger point for the captured samples.
	unsigned int ResponseGetCaptureState::getTriggerPoint() {
		return this->array[2] | (this->array[3] << 8);
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetGain
	/// \brief Sets the data array to needed values.
	CommandSetGain::CommandSetGain() : Helper::DataArray<unsigned char>(8) {
		this->init();
	}
	
	/// \brief Sets the gain to the given values.
	/// \param channel1 The gain value for channel 1.
	/// \param channel2 The gain value for channel 2.
	CommandSetGain::CommandSetGain(unsigned char channel1, unsigned char channel2) : Helper::DataArray<unsigned char>(8) {
		this->init();
		
		this->setGain(0, channel1);
		this->setGain(1, channel2);
	}
	
	/// \brief Get the gain for the given channel.
	/// \param channel The channel whose gain should be returned.
	/// \returns The gain value.
	unsigned char CommandSetGain::getGain(unsigned int channel) {
		GainBits *gainBits = (GainBits *) &(this->array[2]);
		if(channel == 0)
			return gainBits->channel1;
		else
			return gainBits->channel2;
	}
	
	/// \brief Set the gain for the given channel.
	/// \param channel The channel that should be set.
	/// \param value The new gain value for the channel.
	void CommandSetGain::setGain(unsigned int channel, unsigned char value) {
		GainBits *gainBits = (GainBits *) &(this->array[2]);
		if(channel == 0)
			gainBits->channel1 = value;
		else
			gainBits->channel2 = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetGain::init() {
		this->array[0] = COMMAND_SETGAIN;
		this->array[1] = 0x0f;
		((GainBits *) &(this->array[2]))->reserved = 3;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetLogicalData
	/// \brief Sets the data array to needed values.
	CommandSetLogicalData::CommandSetLogicalData() : Helper::DataArray<unsigned char>(8) {
		this->init();
	}
	
	/// \brief Sets the data to the given value.
	/// \param data The data byte.
	CommandSetLogicalData::CommandSetLogicalData(unsigned char data) : Helper::DataArray<unsigned char>(8) {
		this->init();
		
		this->setData(data);
	}
	
	/// \brief Gets the data.
	/// \returns The data byte.
	unsigned char CommandSetLogicalData::getData() {
		return this->array[2];
	}
	
	/// \brief Sets the data to the given value.
	/// \param data The new data byte.
	void CommandSetLogicalData::setData(unsigned char data) {
		this->array[2] = data;
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetLogicalData::init() {
		this->array[0] = COMMAND_SETLOGICALDATA;
		this->array[1] = 0x0f;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandGetLogicalData
	/// \brief Sets the data array to needed values.
	CommandGetLogicalData::CommandGetLogicalData() : Helper::DataArray<unsigned char>(2) {
		this->array[0] = COMMAND_GETLOGICALDATA;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlGetSpeed
	/// \brief Initializes the array.
	ControlGetSpeed::ControlGetSpeed() : Helper::DataArray<unsigned char>(10) {
	}
	
	/// \brief Gets the speed of the connection.
	/// \return The speed level of the USB connection.
	ConnectionSpeed ControlGetSpeed::getSpeed() {
		return (ConnectionSpeed) this->array[0];
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlBeginCommand
	/// \brief Sets the command index to the given value.
	/// \param index The CommandIndex for the command.
	ControlBeginCommand::ControlBeginCommand(CommandIndex index) : Helper::DataArray<unsigned char>(10) {
		this->init();
		
		this->setIndex(index);
	}
	
	/// \brief Gets the command index.
	/// \return The CommandIndex for the command.
	CommandIndex ControlBeginCommand::getIndex() {
		return (CommandIndex) this->array[1];
	}
	
	/// \brief Sets the command index to the given value.
	/// \param index The new CommandIndex for the command.
	void ControlBeginCommand::setIndex(CommandIndex index) {
		memset(&(this->array[1]), (unsigned char) index, 3);
	}
	
	/// \brief Initialize the array to the needed values.
	void ControlBeginCommand::init() {
		this->array[0] = 0x0f;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlSetOffset
	/// \brief Sets the data array to the default values.
	ControlSetOffset::ControlSetOffset() : Helper::DataArray<unsigned char>(17) {
	}
	
	/// \brief Sets the offsets to the given values.
	/// \param channel1 The offset for channel 1.
	/// \param channel2 The offset for channel 2.
	/// \param trigger The offset for ext. trigger.
	ControlSetOffset::ControlSetOffset(unsigned short int channel1, unsigned short int channel2, unsigned short int trigger) : Helper::DataArray<unsigned char>(17) {
		this->setChannel(0, channel1);
		this->setChannel(1, channel2);
		this->setTrigger(trigger);
	}
	
	/// \brief Get the offset for the given channel.
	/// \param channel The channel whose offset should be returned.
	/// \return The channel offset value.
	unsigned short int ControlSetOffset::getChannel(unsigned int channel) {
		if(channel == 0)
			return ((this->array[0] & 0x0f) << 8) | this->array[1];
		else
			return ((this->array[2] & 0x0f) << 8) | this->array[3];
	}
	
	/// \brief Set the offset for the given channel.
	/// \param channel The channel that should be set.
	/// \param offset The new channel offset value.
	void ControlSetOffset::setChannel(unsigned int channel, unsigned short int offset) {
		if(channel == 0) {
			this->array[0] = (unsigned char) (offset >> 8);
			this->array[1] = (unsigned char) offset;
		}
		else {
			this->array[2] = (unsigned char) (offset >> 8);
			this->array[3] = (unsigned char) offset;
		}
	}
	
	/// \brief Get the trigger level.
	/// \return The trigger level value.
	unsigned short int ControlSetOffset::getTrigger() {
		return ((this->array[4] & 0x0f) << 8) | this->array[5];
	}
	
	/// \brief Set the trigger level.
	/// \param level The new trigger level value.
	void ControlSetOffset::setTrigger(unsigned short int level) {
		this->array[4] = (unsigned char) (level >> 8);
		this->array[5] = (unsigned char) level;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlSetRelays
	/// \brief Sets all relay states.
	/// \param ch1Below1V Sets the state of the Channel 1 below 1 V relay.
	/// \param ch1Below100mV Sets the state of the Channel 1 below 100 mV relay.
	/// \param ch1CouplingDC Sets the state of the Channel 1 coupling relay.
	/// \param ch2Below1V Sets the state of the Channel 2 below 1 V relay.
	/// \param ch2Below100mV Sets the state of the Channel 2 below 100 mV relay.
	/// \param ch2CouplingDC Sets the state of the Channel 2 coupling relay.
	/// \param triggerExt Sets the state of the external trigger relay.
	ControlSetRelays::ControlSetRelays(bool ch1Below1V, bool ch1Below100mV, bool ch1CouplingDC, bool ch2Below1V, bool ch2Below100mV, bool ch2CouplingDC, bool triggerExt) : Helper::DataArray<unsigned char>(17) {
		this->setBelow1V(0, ch1Below1V);
		this->setBelow100mV(0, ch1Below100mV);
		this->setCoupling(0, ch1CouplingDC);
		this->setBelow1V(1, ch2Below1V);
		this->setBelow100mV(1, ch2Below100mV);
		this->setCoupling(1, ch2CouplingDC);
		this->setTrigger(triggerExt);
	}
	
	/// \brief Get the below 1 V relay state for the given channel.
	/// \param channel The channel whose relay state should be returned.
	/// \return true, if the gain of the channel is below 1 V.
	bool ControlSetRelays::getBelow1V(unsigned int channel) {
		if(channel == 0)
			return (this->array[1] & 0x04) == 0x00;
		else
			return (this->array[4] & 0x20) == 0x00;
	}
	
	/// \brief Set the below 1 V relay for the given channel.
	/// \param channel The channel that should be set.
	/// \param below true, if the gain of the channel should be below 1 V.
	void ControlSetRelays::setBelow1V(unsigned int channel, bool below) {
		if(channel == 0)
			this->array[1] = below ? 0xfb : 0x04;
		else
			this->array[4] = below ? 0xdf : 0x20;
	}
	
	/// \brief Get the below 1 V relay state for the given channel.
	/// \param channel The channel whose relay state should be returned.
	/// \return true, if the gain of the channel is below 1 V.
	bool ControlSetRelays::getBelow100mV(unsigned int channel) {
		if(channel == 0)
			return (this->array[2] & 0x08) == 0x00;
		else
			return (this->array[5] & 0x40) == 0x00;
	}
	
	/// \brief Set the below 100 mV relay for the given channel.
	/// \param channel The channel that should be set.
	/// \param below true, if the gain of the channel should be below 100 mV.
	void ControlSetRelays::setBelow100mV(unsigned int channel, bool below) {
		if(channel == 0)
			this->array[2] = below ? 0xf7 : 0x08;
		else
			this->array[5] = below ? 0xbf : 0x40;
	}
	
	/// \brief Get the coupling relay state for the given channel.
	/// \param channel The channel whose relay state should be returned.
	/// \return true, if the coupling of the channel is DC.
	bool ControlSetRelays::getCoupling(unsigned int channel) {
		if(channel == 0)
			return (this->array[3] & 0x02) == 0x00;
		else
			return (this->array[6] & 0x10) == 0x00;
	}
	
	/// \brief Set the coupling relay for the given channel.
	/// \param channel The channel that should be set.
	/// \param dc true, if the coupling of the channel should be DC.
	void ControlSetRelays::setCoupling(unsigned int channel, bool dc) {
		if(channel == 0)
			this->array[3] = dc ? 0xfd : 0x02;
		else
			this->array[6] = dc ? 0xef : 0x10;
	}
	
	/// \brief Get the external trigger relay state.
	/// \return true, if the trigger is external (EXT-Connector).
	bool ControlSetRelays::getTrigger() {
		return (this->array[7] & 0x01) == 0x00;
	}
	
	/// \brief Set the external trigger relay.
	/// \param ext true, if the trigger should be external (EXT-Connector).
	void ControlSetRelays::setTrigger(bool ext) {
		this->array[7] = ext ? 0xfe : 0x01;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetSamplerate5200
	/// \brief Sets the data array to the default values.
	CommandSetSamplerate5200::CommandSetSamplerate5200() : Helper::DataArray<unsigned char>(6) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param samplerateSlow The SamplerateSlow value.
	/// \param samplerateFast The SamplerateFast value.
	CommandSetSamplerate5200::CommandSetSamplerate5200(unsigned short int samplerateSlow, unsigned char samplerateFast) : Helper::DataArray<unsigned char>(6) {
		this->init();
		
		this->setSamplerateFast(samplerateFast);
		this->setSamplerateSlow(samplerateSlow);
	}
	
	/// \brief Get the SamplerateFast value.
	/// \return The SamplerateFast value.
	unsigned char CommandSetSamplerate5200::getSamplerateFast() {
		return this->array[4];
	}
	
	/// \brief Set the SamplerateFast to the given value.
	/// \param value The new SamplerateFast value.
	void CommandSetSamplerate5200::setSamplerateFast(unsigned char value) {
		this->array[4] = value;
	}
	
	/// \brief Get the SamplerateSlow value.
	/// \return The SamplerateSlow value.
	unsigned short int CommandSetSamplerate5200::getSamplerateSlow() {
		return (unsigned short int) this->array[2] | ((unsigned short int) this->array[3] << 8);
	}
	
	/// \brief Set the SamplerateSlow to the given value.
	/// \param samplerate The new SamplerateSlow value.
	void CommandSetSamplerate5200::setSamplerateSlow(unsigned short int samplerate) {
		this->array[2] = (unsigned char) samplerate;
		this->array[3] = (unsigned char) (samplerate >> 8);
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetSamplerate5200::init() {
		this->array[0] = COMMAND_SETSAMPLERATE5200;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetBuffer5200
	/// \brief Sets the data array to the default values.
	CommandSetBuffer5200::CommandSetBuffer5200() : Helper::DataArray<unsigned char>(10) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param triggerPositionPre The TriggerPositionPre value.
	/// \param triggerPositionPost The TriggerPositionPost value.
	/// \param usedPre The TriggerPositionUsedPre value.
	/// \param usedPost The TriggerPositionUsedPost value.
	/// \param bufferSize The #BufferSizeId value.
	CommandSetBuffer5200::CommandSetBuffer5200(unsigned short int triggerPositionPre, unsigned short int triggerPositionPost, unsigned char usedPre, unsigned char usedPost, unsigned char bufferSize) : Helper::DataArray<unsigned char>(10) {
		this->init();
		
		this->setTriggerPositionPre(triggerPositionPre);
		this->setTriggerPositionPost(triggerPositionPost);
		this->setUsedPre(usedPre);
		this->setUsedPost(usedPost);
		this->setBufferSize(bufferSize);
	}
	
	/// \brief Get the TriggerPositionPre value.
	/// \return The TriggerPositionPre value.
	unsigned short int CommandSetBuffer5200::getTriggerPositionPre() {
		return (unsigned short int) this->array[2] | ((unsigned short int) this->array[3] << 8);
	}
	
	/// \brief Set the TriggerPositionPre to the given value.
	/// \param position The new TriggerPositionPre value.
	void CommandSetBuffer5200::setTriggerPositionPre(unsigned short int position) {
		this->array[2] = (unsigned char) position;
		this->array[3] = (unsigned char) (position >> 8);
	}
	
	/// \brief Get the TriggerPositionPost value.
	/// \return The TriggerPositionPost value.
	unsigned short int CommandSetBuffer5200::getTriggerPositionPost() {
		return (unsigned short int) this->array[6] | ((unsigned short int) this->array[7] << 8);
	}
	
	/// \brief Set the TriggerPositionPost to the given value.
	/// \param position The new TriggerPositionPost value.
	void CommandSetBuffer5200::setTriggerPositionPost(unsigned short int position) {
		this->array[6] = (unsigned char) position;
		this->array[7] = (unsigned char) (position >> 8);
	}
	
	/// \brief Get the TriggerPositionUsedPre value.
	/// \return The #DTriggerPositionUsed value for the pre position.
	unsigned char CommandSetBuffer5200::getUsedPre() {
		return this->array[4];
	}
	
	/// \brief Set the TriggerPositionUsedPre to the given value.
	/// \param value The new #DTriggerPositionUsed value for the pre position.
	void CommandSetBuffer5200::setUsedPre(unsigned char value) {
		this->array[4] = value;
	}
	
	/// \brief Get the TriggerPositionUsedPost value.
	/// \return The #DTriggerPositionUsed value for the post position.
	unsigned char CommandSetBuffer5200::getUsedPost() {
		return ((DBufferBits *) &(this->array[8]))->triggerPositionUsed;
	}
	
	/// \brief Set the TriggerPositionUsedPost to the given value.
	/// \param value The new #DTriggerPositionUsed value for the post position.
	void CommandSetBuffer5200::setUsedPost(unsigned char value) {
		((DBufferBits *) &(this->array[8]))->triggerPositionUsed = value;
	}
	
	/// \brief Get the bufferSize value in DBufferBits.
	/// \return The #BufferSizeId value.
	unsigned char CommandSetBuffer5200::getBufferSize() {
		return ((DBufferBits *) &(this->array[8]))->bufferSize;
	}
	
	/// \brief Set the bufferSize in DBufferBits to the given value.
	/// \param value The new #BufferSizeId value.
	void CommandSetBuffer5200::setBufferSize(unsigned char value) {
		((DBufferBits *) &(this->array[8]))->bufferSize = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetBuffer5200::init() {
		this->array[0] = COMMAND_SETBUFFER5200;
		this->array[5] = 0xff;
		this->array[9] = 0xff;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class CommandSetTrigger5200
	/// \brief Sets the data array to the default values.
	CommandSetTrigger5200::CommandSetTrigger5200() : Helper::DataArray<unsigned char>(8) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param triggerSource The trigger source id.
	/// \param usedChannels The enabled channels.
	/// \param fastRate The fastRate state.
	/// \param triggerSlope The triggerSlope value.
	/// \param triggerPulse The triggerPulse value.
	CommandSetTrigger5200::CommandSetTrigger5200(unsigned char triggerSource, unsigned char usedChannels, bool fastRate, unsigned char triggerSlope, unsigned char triggerPulse) : Helper::DataArray<unsigned char>(8) {
		this->init();
		
		this->setTriggerSource(triggerSource);
		this->setUsedChannels(usedChannels);
		this->setFastRate(fastRate);
		this->setTriggerSlope(triggerSlope);
		this->setTriggerPulse(triggerPulse);
	}
	
	/// \brief Get the triggerSource value in ETsrBits.
	/// \return The #TriggerSource value.
	unsigned char CommandSetTrigger5200::getTriggerSource() {
		return ((ETsrBits *) &(this->array[2]))->triggerSource;
	}
	
	/// \brief Set the triggerSource in ETsrBits to the given value.
	/// \param value The new #TriggerSource value.
	void CommandSetTrigger5200::setTriggerSource(unsigned char value) {
		((ETsrBits *) &(this->array[2]))->triggerSource = value;
	}
	
	/// \brief Get the usedChannels value in ETsrBits.
	/// \return The #UsedChannels value.
	unsigned char CommandSetTrigger5200::getUsedChannels() {
		return ((ETsrBits *) &(this->array[2]))->usedChannels;
	}
	
	/// \brief Set the usedChannels in ETsrBits to the given value.
	/// \param value The new #UsedChannels value.
	void CommandSetTrigger5200::setUsedChannels(unsigned char value) {
		((ETsrBits *) &(this->array[2]))->usedChannels = value;
	}
	
	/// \brief Get the fastRate state in ETsrBits.
	/// \return The fastRate state (Already inverted).
	bool CommandSetTrigger5200::getFastRate() {
		return ((ETsrBits *) &(this->array[2]))->fastRate == 0;
	}
	
	/// \brief Set the fastRate in ETsrBits to the given state.
	/// \param fastRate The new fastRate state (Automatically inverted).
	void CommandSetTrigger5200::setFastRate(bool fastRate) {
		((ETsrBits *) &(this->array[2]))->fastRate = fastRate ? 0 : 1;
	}
	
	/// \brief Get the triggerSlope value in ETsrBits.
	/// \return The triggerSlope value.
	unsigned char CommandSetTrigger5200::getTriggerSlope() {
		return ((ETsrBits *) &(this->array[2]))->triggerSlope;
	}
	
	/// \brief Set the triggerSlope in ETsrBits to the given value.
	/// \param slope The new triggerSlope value.
	void CommandSetTrigger5200::setTriggerSlope(unsigned char slope) {
		((ETsrBits *) &(this->array[2]))->triggerSlope = slope;
	}
	
	/// \brief Get the triggerPulse state in ETsrBits.
	/// \return The triggerPulse state.
	bool CommandSetTrigger5200::getTriggerPulse() {
		return ((ETsrBits *) &(this->array[2]))->triggerPulse == 1;
	}
	
	/// \brief Set the triggerPulse in ETsrBits to the given state.
	/// \param pulse The new triggerPulse state.
	void CommandSetTrigger5200::setTriggerPulse(bool pulse) {
		((ETsrBits *) &(this->array[2]))->triggerPulse = pulse ? 1 : 0;
	}
	
	/// \brief Initialize the array to the needed values.
	void CommandSetTrigger5200::init() {
		this->array[0] = COMMAND_SETTRIGGER5200;
		this->array[4] = 0x02;
	}
}
