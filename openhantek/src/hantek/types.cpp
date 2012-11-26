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
	// class BulkSetFilter
	/// \brief Sets the data array to the default values.
	BulkSetFilter::BulkSetFilter() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the FilterByte to the given value.
	/// \param channel1 true if channel 1 is filtered.
	/// \param channel2 true if channel 2 is filtered.
	/// \param trigger true if trigger is filtered.
	BulkSetFilter::BulkSetFilter(bool channel1, bool channel2, bool trigger) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setChannel(0, channel1);
		this->setChannel(1, channel2);
		this->setTrigger(trigger);
	}
	
	/// \brief Gets the filtering state of one channel.
	/// \param channel The channel whose filtering state should be returned.
	/// \return The filtering state of the channel.
	bool BulkSetFilter::getChannel(unsigned int channel) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		if(channel == 0)
			return filterBits->channel1 == 1;
		else
			return filterBits->channel2 == 1;
	}
	
	/// \brief Enables/disables filtering of one channel.
	/// \param channel The channel that should be set.
	/// \param filtered true if the channel should be filtered.
	void BulkSetFilter::setChannel(unsigned int channel, bool filtered) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		if(channel == 0)
			filterBits->channel1 = filtered ? 1 : 0;
		else
			filterBits->channel2 = filtered ? 1 : 0;
	}
	
	/// \brief Gets the filtering state for the trigger.
	/// \return The filtering state of the trigger.
	bool BulkSetFilter::getTrigger() {
		return ((FilterBits *) &(this->array[2]))->trigger == 1;
	}
	
	/// \brief Enables/disables filtering for the trigger.
	/// \param filtered true if the trigger should be filtered.
	void BulkSetFilter::setTrigger(bool filtered) {
		FilterBits *filterBits = (FilterBits *) &(this->array[2]);
		
		filterBits->trigger = filtered ? 1 : 0;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetFilter::init() {
		this->array[0] = BULK_SETFILTER;
		this->array[1] = 0x0f;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetTriggerAndSamplerate
	/// \brief Sets the data array to the default values.
	BulkSetTriggerAndSamplerate::BulkSetTriggerAndSamplerate() : Helper::DataArray<uint8_t>(12) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param downsampler The Downsampler value.
	/// \param triggerPosition The trigger position value.
	/// \param triggerSource The trigger source id (Tsr1).
	/// \param recordLength The record length id (Tsr1).
	/// \param samplerateId The samplerateId value (Tsr1).
	/// \param downsamplingMode The downsamplingMode value (Tsr1).
	/// \param usedChannels The enabled channels (Tsr2).
	/// \param fastRate The fastRate state (Tsr2).
	/// \param triggerSlope The triggerSlope value (Tsr2).
	BulkSetTriggerAndSamplerate::BulkSetTriggerAndSamplerate(uint16_t downsampler, uint32_t triggerPosition, uint8_t triggerSource, uint8_t recordLength, uint8_t samplerateId, bool downsamplingMode, uint8_t usedChannels, bool fastRate, uint8_t triggerSlope) : Helper::DataArray<uint8_t>(12) {
		this->init();
		
		this->setTriggerSource(triggerSource);
		this->setRecordLength(recordLength);
		this->setSamplerateId(samplerateId);
		this->setDownsamplingMode(downsamplingMode);
		this->setUsedChannels(usedChannels);
		this->setFastRate(fastRate);
		this->setTriggerSlope(triggerSlope);
		this->setDownsampler(downsampler);
		this->setTriggerPosition(triggerPosition);
	}
	
	/// \brief Get the triggerSource value in Tsr1Bits.
	/// \return The triggerSource value.
	uint8_t BulkSetTriggerAndSamplerate::getTriggerSource() {
		return ((Tsr1Bits *) &(this->array[2]))->triggerSource;
	}
	
	/// \brief Set the triggerSource in Tsr1Bits to the given value.
	/// \param value The new triggerSource value.
	void BulkSetTriggerAndSamplerate::setTriggerSource(uint8_t value) {
		((Tsr1Bits *) &(this->array[2]))->triggerSource = value;
	}
	
	/// \brief Get the recordLength value in Tsr1Bits.
	/// \return The ::RecordLengthId value.
	uint8_t BulkSetTriggerAndSamplerate::getRecordLength() {
		return ((Tsr1Bits *) &(this->array[2]))->recordLength;
	}
	
	/// \brief Set the recordLength in Tsr1Bits to the given value.
	/// \param value The new ::RecordLengthId value.
	void BulkSetTriggerAndSamplerate::setRecordLength(uint8_t value) {
		((Tsr1Bits *) &(this->array[2]))->recordLength = value;
	}
	
	/// \brief Get the samplerateId value in Tsr1Bits.
	/// \return The samplerateId value.
	uint8_t BulkSetTriggerAndSamplerate::getSamplerateId() {
		return ((Tsr1Bits *) &(this->array[2]))->samplerateId;
	}
	
	/// \brief Set the samplerateId in Tsr1Bits to the given value.
	/// \param value The new samplerateId value.
	void BulkSetTriggerAndSamplerate::setSamplerateId(uint8_t value) {
		((Tsr1Bits *) &(this->array[2]))->samplerateId = value;
	}
	
	/// \brief Get the downsamplerMode value in Tsr1Bits.
	/// \return The downsamplerMode value.
	bool BulkSetTriggerAndSamplerate::getDownsamplingMode() {
		return ((Tsr1Bits *) &(this->array[2]))->downsamplingMode == 1;
	}
	
	/// \brief Set the downsamplerMode in Tsr1Bits to the given value.
	/// \param downsampling The new downsamplerMode value.
	void BulkSetTriggerAndSamplerate::setDownsamplingMode(bool downsampling) {
		((Tsr1Bits *) &(this->array[2]))->downsamplingMode = downsampling ? 1 : 0;
	}
	
	/// \brief Get the usedChannels value in Tsr2Bits.
	/// \return The usedChannels value.
	uint8_t BulkSetTriggerAndSamplerate::getUsedChannels() {
		return ((Tsr2Bits *) &(this->array[3]))->usedChannels;
	}
	
	/// \brief Set the usedChannels in Tsr2Bits to the given value.
	/// \param value The new usedChannels value.
	void BulkSetTriggerAndSamplerate::setUsedChannels(uint8_t value) {
		((Tsr2Bits *) &(this->array[3]))->usedChannels = value;
	}
	
	/// \brief Get the fastRate state in Tsr2Bits.
	/// \return The fastRate state.
	bool BulkSetTriggerAndSamplerate::getFastRate() {
		return ((Tsr2Bits *) &(this->array[3]))->fastRate == 1;
	}
	
	/// \brief Set the fastRate in Tsr2Bits to the given state.
	/// \param fastRate The new fastRate state.
	void BulkSetTriggerAndSamplerate::setFastRate(bool fastRate) {
		((Tsr2Bits *) &(this->array[3]))->fastRate = fastRate ? 1 : 0;
	}
	
	/// \brief Get the triggerSlope value in Tsr2Bits.
	/// \return The triggerSlope value.
	uint8_t BulkSetTriggerAndSamplerate::getTriggerSlope() {
		return ((Tsr2Bits *) &(this->array[3]))->triggerSlope;
	}
	
	/// \brief Set the triggerSlope in Tsr2Bits to the given value.
	/// \param slope The new triggerSlope value.
	void BulkSetTriggerAndSamplerate::setTriggerSlope(uint8_t slope) {
		((Tsr2Bits *) &(this->array[3]))->triggerSlope = slope;
	}
	
	/// \brief Get the Downsampler value.
	/// \return The Downsampler value.
	uint16_t BulkSetTriggerAndSamplerate::getDownsampler() {
		return (uint16_t) this->array[4] | ((uint16_t) this->array[5] << 8);
	}
	
	/// \brief Set the Downsampler to the given value.
	/// \param downsampler The new Downsampler value.
	void BulkSetTriggerAndSamplerate::setDownsampler(uint16_t downsampler) {
		this->array[4] = (uint8_t) downsampler;
		this->array[5] = (uint8_t) (downsampler >> 8);
	}
	
	/// \brief Get the TriggerPosition value.
	/// \return The horizontal trigger position.
	uint32_t BulkSetTriggerAndSamplerate::getTriggerPosition() {
		return (uint32_t) this->array[6] | ((uint32_t) this->array[7] << 8) | ((uint32_t) this->array[10] << 16);
	}
	
	/// \brief Set the TriggerPosition to the given value.
	/// \param position The new horizontal trigger position.
	void BulkSetTriggerAndSamplerate::setTriggerPosition(uint32_t position) {
		this->array[6] = (uint8_t) position;
		this->array[7] = (uint8_t) (position >> 8);
		this->array[10] = (uint8_t) (position >> 16);
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetTriggerAndSamplerate::init() {
		this->array[0] = BULK_SETTRIGGERANDSAMPLERATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkForceTrigger
	/// \brief Sets the data array to needed values.
	BulkForceTrigger::BulkForceTrigger() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_FORCETRIGGER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkCaptureStart
	/// \brief Sets the data array to needed values.
	BulkCaptureStart::BulkCaptureStart() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_STARTSAMPLING;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkTriggerEnabled
	/// \brief Sets the data array to needed values.
	BulkTriggerEnabled::BulkTriggerEnabled() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_ENABLETRIGGER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkGetData
	/// \brief Sets the data array to needed values.
	BulkGetData::BulkGetData() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_GETDATA;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkGetCaptureState
	/// \brief Sets the data array to needed values.
	BulkGetCaptureState::BulkGetCaptureState() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_GETCAPTURESTATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkResponseGetCaptureState
	/// \brief Initializes the array.
	BulkResponseGetCaptureState::BulkResponseGetCaptureState() : Helper::DataArray<uint8_t>(512) {
	}
	
	/// \brief Gets the capture state.
	/// \return The CaptureState of the oscilloscope.
	CaptureState BulkResponseGetCaptureState::getCaptureState() {
		return (CaptureState) this->array[0];
	}
	
	/// \brief Gets the trigger point.
	/// \return The trigger point for the captured samples.
	unsigned int BulkResponseGetCaptureState::getTriggerPoint() {
		return this->array[2] | (this->array[3] << 8) | (this->array[1] << 16);
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetGain
	/// \brief Sets the data array to needed values.
	BulkSetGain::BulkSetGain() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the gain to the given values.
	/// \param channel1 The gain value for channel 1.
	/// \param channel2 The gain value for channel 2.
	BulkSetGain::BulkSetGain(uint8_t channel1, uint8_t channel2) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setGain(0, channel1);
		this->setGain(1, channel2);
	}
	
	/// \brief Get the gain for the given channel.
	/// \param channel The channel whose gain should be returned.
	/// \returns The gain value.
	uint8_t BulkSetGain::getGain(unsigned int channel) {
		GainBits *gainBits = (GainBits *) &(this->array[2]);
		if(channel == 0)
			return gainBits->channel1;
		else
			return gainBits->channel2;
	}
	
	/// \brief Set the gain for the given channel.
	/// \param channel The channel that should be set.
	/// \param value The new gain value for the channel.
	void BulkSetGain::setGain(unsigned int channel, uint8_t value) {
		GainBits *gainBits = (GainBits *) &(this->array[2]);
		if(channel == 0)
			gainBits->channel1 = value;
		else
			gainBits->channel2 = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetGain::init() {
		this->array[0] = BULK_SETGAIN;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetLogicalData
	/// \brief Sets the data array to needed values.
	BulkSetLogicalData::BulkSetLogicalData() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the data to the given value.
	/// \param data The data byte.
	BulkSetLogicalData::BulkSetLogicalData(uint8_t data) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setData(data);
	}
	
	/// \brief Gets the data.
	/// \returns The data byte.
	uint8_t BulkSetLogicalData::getData() {
		return this->array[2];
	}
	
	/// \brief Sets the data to the given value.
	/// \param data The new data byte.
	void BulkSetLogicalData::setData(uint8_t data) {
		this->array[2] = data;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetLogicalData::init() {
		this->array[0] = BULK_SETLOGICALDATA;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkGetLogicalData
	/// \brief Sets the data array to needed values.
	BulkGetLogicalData::BulkGetLogicalData() : Helper::DataArray<uint8_t>(2) {
		this->array[0] = BULK_GETLOGICALDATA;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetFilter2250
	/// \brief Sets the data array to needed values.
	BulkSetChannels2250::BulkSetChannels2250() : Helper::DataArray<uint8_t>(4) {
		this->init();
	}
	
	/// \brief Sets the used channels.
	/// \param usedChannels The UsedChannels value.
	BulkSetChannels2250::BulkSetChannels2250(uint8_t usedChannels) : Helper::DataArray<uint8_t>(4) {
		this->init();
		
		this->setUsedChannels(usedChannels);
	}
	
	/// \brief Get the UsedChannels value
	/// \return The UsedChannels value.
	uint8_t BulkSetChannels2250::getUsedChannels() {
		return this->array[2];
	}
	
	/// \brief Set the UsedChannels to the given value.
	/// \param value The new UsedChannels value.
	void BulkSetChannels2250::setUsedChannels(uint8_t value) {
		this->array[2] = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetChannels2250::init() {
		this->array[0] = BULK_BSETCHANNELS;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetTrigger2250
	/// \brief Sets the data array to needed values.
	BulkSetTrigger2250::BulkSetTrigger2250() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the used channels.
	/// \param triggerSource The trigger source id (CTriggerBits).
	/// \param triggerSlope The triggerSlope value (CTriggerBits).
	BulkSetTrigger2250::BulkSetTrigger2250(uint8_t triggerSource, uint8_t triggerSlope) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setTriggerSource(triggerSource);
		this->setTriggerSlope(triggerSlope);
	}
			
	/// \brief Get the triggerSource value in CTriggerBits.
	/// \return The triggerSource value.
	uint8_t BulkSetTrigger2250::getTriggerSource() {
		return ((CTriggerBits *) &(this->array[2]))->triggerSource;
	}
	
	/// \brief Set the triggerSource in CTriggerBits to the given value.
	/// \param value The new triggerSource value.
	void BulkSetTrigger2250::setTriggerSource(uint8_t value) {
		((CTriggerBits *) &(this->array[2]))->triggerSource = value;
	}
	
	/// \brief Get the triggerSlope value in CTriggerBits.
	/// \return The triggerSlope value.
	uint8_t BulkSetTrigger2250::getTriggerSlope() {
		return ((CTriggerBits *) &(this->array[2]))->triggerSlope;
	}
	
	/// \brief Set the triggerSlope in CTriggerBits to the given value.
	/// \param slope The new triggerSlope value.
	void BulkSetTrigger2250::setTriggerSlope(uint8_t slope) {
		((CTriggerBits *) &(this->array[2]))->triggerSlope = slope;
	}
		
	/// \brief Initialize the array to the needed values.
	void BulkSetTrigger2250::init() {
		this->array[0] = BULK_CSETTRIGGERORSAMPLERATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetSamplerate5200
	/// \brief Sets the data array to the default values.
	BulkSetSamplerate5200::BulkSetSamplerate5200() : Helper::DataArray<uint8_t>(6) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param samplerateSlow The SamplerateSlow value.
	/// \param samplerateFast The SamplerateFast value.
	BulkSetSamplerate5200::BulkSetSamplerate5200(uint16_t samplerateSlow, uint8_t samplerateFast) : Helper::DataArray<uint8_t>(6) {
		this->init();
		
		this->setSamplerateFast(samplerateFast);
		this->setSamplerateSlow(samplerateSlow);
	}
	
	/// \brief Get the SamplerateFast value.
	/// \return The SamplerateFast value.
	uint8_t BulkSetSamplerate5200::getSamplerateFast() {
		return this->array[4];
	}
	
	/// \brief Set the SamplerateFast to the given value.
	/// \param value The new SamplerateFast value.
	void BulkSetSamplerate5200::setSamplerateFast(uint8_t value) {
		this->array[4] = value;
	}
	
	/// \brief Get the SamplerateSlow value.
	/// \return The SamplerateSlow value.
	uint16_t BulkSetSamplerate5200::getSamplerateSlow() {
		return (uint16_t) this->array[2] | ((uint16_t) this->array[3] << 8);
	}
	
	/// \brief Set the SamplerateSlow to the given value.
	/// \param samplerate The new SamplerateSlow value.
	void BulkSetSamplerate5200::setSamplerateSlow(uint16_t samplerate) {
		this->array[2] = (uint8_t) samplerate;
		this->array[3] = (uint8_t) (samplerate >> 8);
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetSamplerate5200::init() {
		this->array[0] = BULK_CSETTRIGGERORSAMPLERATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetBuffer2250
	/// \brief Sets the data array to the default values.
	BulkSetRecordLength2250::BulkSetRecordLength2250() : Helper::DataArray<uint8_t>(4) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param recordLength The ::RecordLengthId value.
	BulkSetRecordLength2250::BulkSetRecordLength2250(uint8_t recordLength) : Helper::DataArray<uint8_t>(4) {
		this->init();
		
		this->setRecordLength(recordLength);
	}
	
	/// \brief Get the ::RecordLengthId value.
	/// \return The ::RecordLengthId value.
	uint8_t BulkSetRecordLength2250::getRecordLength() {
		return this->array[2];
	}
	
	/// \brief Set the ::RecordLengthId to the given value.
	/// \param value The new ::RecordLengthId value.
	void BulkSetRecordLength2250::setRecordLength(uint8_t value) {
		this->array[2] = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetRecordLength2250::init() {
		this->array[0] = BULK_DSETBUFFER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetBuffer5200
	/// \brief Sets the data array to the default values.
	BulkSetBuffer5200::BulkSetBuffer5200() : Helper::DataArray<uint8_t>(10) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param triggerPositionPre The TriggerPositionPre value.
	/// \param triggerPositionPost The TriggerPositionPost value.
	/// \param usedPre The TriggerPositionUsedPre value.
	/// \param usedPost The TriggerPositionUsedPost value.
	/// \param recordLength The ::RecordLengthId value.
	BulkSetBuffer5200::BulkSetBuffer5200(uint16_t triggerPositionPre, uint16_t triggerPositionPost, uint8_t usedPre, uint8_t usedPost, uint8_t recordLength) : Helper::DataArray<uint8_t>(10) {
		this->init();
		
		this->setTriggerPositionPre(triggerPositionPre);
		this->setTriggerPositionPost(triggerPositionPost);
		this->setUsedPre(usedPre);
		this->setUsedPost(usedPost);
		this->setRecordLength(recordLength);
	}
	
	/// \brief Get the TriggerPositionPre value.
	/// \return The TriggerPositionPre value.
	uint16_t BulkSetBuffer5200::getTriggerPositionPre() {
		return (uint16_t) this->array[2] | ((uint16_t) this->array[3] << 8);
	}
	
	/// \brief Set the TriggerPositionPre to the given value.
	/// \param position The new TriggerPositionPre value.
	void BulkSetBuffer5200::setTriggerPositionPre(uint16_t position) {
		this->array[2] = (uint8_t) position;
		this->array[3] = (uint8_t) (position >> 8);
	}
	
	/// \brief Get the TriggerPositionPost value.
	/// \return The TriggerPositionPost value.
	uint16_t BulkSetBuffer5200::getTriggerPositionPost() {
		return (uint16_t) this->array[6] | ((uint16_t) this->array[7] << 8);
	}
	
	/// \brief Set the TriggerPositionPost to the given value.
	/// \param position The new TriggerPositionPost value.
	void BulkSetBuffer5200::setTriggerPositionPost(uint16_t position) {
		this->array[6] = (uint8_t) position;
		this->array[7] = (uint8_t) (position >> 8);
	}
	
	/// \brief Get the TriggerPositionUsedPre value.
	/// \return The ::DTriggerPositionUsed value for the pre position.
	uint8_t BulkSetBuffer5200::getUsedPre() {
		return this->array[4];
	}
	
	/// \brief Set the TriggerPositionUsedPre to the given value.
	/// \param value The new ::DTriggerPositionUsed value for the pre position.
	void BulkSetBuffer5200::setUsedPre(uint8_t value) {
		this->array[4] = value;
	}
	
	/// \brief Get the TriggerPositionUsedPost value.
	/// \return The ::DTriggerPositionUsed value for the post position.
	uint8_t BulkSetBuffer5200::getUsedPost() {
		return ((DBufferBits *) &(this->array[8]))->triggerPositionUsed;
	}
	
	/// \brief Set the TriggerPositionUsedPost to the given value.
	/// \param value The new ::DTriggerPositionUsed value for the post position.
	void BulkSetBuffer5200::setUsedPost(uint8_t value) {
		((DBufferBits *) &(this->array[8]))->triggerPositionUsed = value;
	}
	
	/// \brief Get the recordLength value in DBufferBits.
	/// \return The ::RecordLengthId value.
	uint8_t BulkSetBuffer5200::getRecordLength() {
		return ((DBufferBits *) &(this->array[8]))->recordLength;
	}
	
	/// \brief Set the recordLength in DBufferBits to the given value.
	/// \param value The new ::RecordLengthId value.
	void BulkSetBuffer5200::setRecordLength(uint8_t value) {
		((DBufferBits *) &(this->array[8]))->recordLength = value;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetBuffer5200::init() {
		this->array[0] = BULK_DSETBUFFER;
		this->array[5] = 0xff;
		this->array[9] = 0xff;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetSamplerate2250
	/// \brief Sets the data array to the default values.
	BulkSetSamplerate2250::BulkSetSamplerate2250() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param fastRate The fastRate state (ESamplerateBits).
	/// \param downsampling The downsampling state (ESamplerateBits).
	/// \param samplerate The Samplerate value.
	BulkSetSamplerate2250::BulkSetSamplerate2250(bool fastRate, bool downsampling, uint16_t samplerate) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setFastRate(fastRate);
		this->setDownsampling(downsampling);
		this->setSamplerate(samplerate);
	}
	
	/// \brief Get the fastRate state in ESamplerateBits.
	/// \return The fastRate state.
	bool BulkSetSamplerate2250::getFastRate() {
		return ((ESamplerateBits *) &(this->array[2]))->fastRate == 1;
	}
	
	/// \brief Set the fastRate in ESamplerateBits to the given state.
	/// \param fastRate The new fastRate state.
	void BulkSetSamplerate2250::setFastRate(bool fastRate) {
		((ESamplerateBits *) &(this->array[2]))->fastRate = fastRate ? 1 : 0;
	}
	
	/// \brief Get the downsampling state in ESamplerateBits.
	/// \return The downsampling state.
	bool BulkSetSamplerate2250::getDownsampling() {
		return ((ESamplerateBits *) &(this->array[2]))->downsampling == 1;
	}
	
	/// \brief Set the downsampling in ESamplerateBits to the given state.
	/// \param downsampling The new downsampling state.
	void BulkSetSamplerate2250::setDownsampling(bool downsampling) {
		((ESamplerateBits *) &(this->array[2]))->downsampling = downsampling ? 1 : 0;
	}
	
	/// \brief Get the Samplerate value.
	/// \return The Samplerate value.
	uint16_t BulkSetSamplerate2250::getSamplerate() {
		return (uint16_t) this->array[4] | ((uint16_t) this->array[5] << 8);
	}
	
	/// \brief Set the Samplerate to the given value.
	/// \param samplerate The new Samplerate value.
	void BulkSetSamplerate2250::setSamplerate(uint16_t samplerate) {
		this->array[4] = (uint8_t) samplerate;
		this->array[5] = (uint8_t) (samplerate >> 8);
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetSamplerate2250::init() {
		this->array[0] = BULK_ESETTRIGGERORSAMPLERATE;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class BulkSetTrigger5200
	/// \brief Sets the data array to the default values.
	BulkSetTrigger5200::BulkSetTrigger5200() : Helper::DataArray<uint8_t>(8) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param triggerSource The trigger source id.
	/// \param usedChannels The enabled channels.
	/// \param fastRate The fastRate state.
	/// \param triggerSlope The triggerSlope value.
	/// \param triggerPulse The triggerPulse value.
	BulkSetTrigger5200::BulkSetTrigger5200(uint8_t triggerSource, uint8_t usedChannels, bool fastRate, uint8_t triggerSlope, uint8_t triggerPulse) : Helper::DataArray<uint8_t>(8) {
		this->init();
		
		this->setTriggerSource(triggerSource);
		this->setUsedChannels(usedChannels);
		this->setFastRate(fastRate);
		this->setTriggerSlope(triggerSlope);
		this->setTriggerPulse(triggerPulse);
	}
	
	/// \brief Get the triggerSource value in ETsrBits.
	/// \return The ::TriggerSource value.
	uint8_t BulkSetTrigger5200::getTriggerSource() {
		return ((ETsrBits *) &(this->array[2]))->triggerSource;
	}
	
	/// \brief Set the triggerSource in ETsrBits to the given value.
	/// \param value The new ::TriggerSource value.
	void BulkSetTrigger5200::setTriggerSource(uint8_t value) {
		((ETsrBits *) &(this->array[2]))->triggerSource = value;
	}
	
	/// \brief Get the usedChannels value in ETsrBits.
	/// \return The ::UsedChannels value.
	uint8_t BulkSetTrigger5200::getUsedChannels() {
		return ((ETsrBits *) &(this->array[2]))->usedChannels;
	}
	
	/// \brief Set the usedChannels in ETsrBits to the given value.
	/// \param value The new ::UsedChannels value.
	void BulkSetTrigger5200::setUsedChannels(uint8_t value) {
		((ETsrBits *) &(this->array[2]))->usedChannels = value;
	}
	
	/// \brief Get the fastRate state in ETsrBits.
	/// \return The fastRate state (Already inverted).
	bool BulkSetTrigger5200::getFastRate() {
		return ((ETsrBits *) &(this->array[2]))->fastRate == 0;
	}
	
	/// \brief Set the fastRate in ETsrBits to the given state.
	/// \param fastRate The new fastRate state (Automatically inverted).
	void BulkSetTrigger5200::setFastRate(bool fastRate) {
		((ETsrBits *) &(this->array[2]))->fastRate = fastRate ? 0 : 1;
	}
	
	/// \brief Get the triggerSlope value in ETsrBits.
	/// \return The triggerSlope value.
	uint8_t BulkSetTrigger5200::getTriggerSlope() {
		return ((ETsrBits *) &(this->array[2]))->triggerSlope;
	}
	
	/// \brief Set the triggerSlope in ETsrBits to the given value.
	/// \param slope The new triggerSlope value.
	void BulkSetTrigger5200::setTriggerSlope(uint8_t slope) {
		((ETsrBits *) &(this->array[2]))->triggerSlope = slope;
	}
	
	/// \brief Get the triggerPulse state in ETsrBits.
	/// \return The triggerPulse state.
	bool BulkSetTrigger5200::getTriggerPulse() {
		return ((ETsrBits *) &(this->array[2]))->triggerPulse == 1;
	}
	
	/// \brief Set the triggerPulse in ETsrBits to the given state.
	/// \param pulse The new triggerPulse state.
	void BulkSetTrigger5200::setTriggerPulse(bool pulse) {
		((ETsrBits *) &(this->array[2]))->triggerPulse = pulse ? 1 : 0;
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetTrigger5200::init() {
		this->array[0] = BULK_ESETTRIGGERORSAMPLERATE;
		this->array[4] = 0x02;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class BulkSetBuffer2250                                    hantek/types.h
	/// \brief The DSO-2250 BULK_FSETBUFFER builder.
	/// \brief Sets the data array to the default values.
	BulkSetBuffer2250::BulkSetBuffer2250() : Helper::DataArray<uint8_t>(10) {
		this->init();
	}
	
	/// \brief Sets the data bytes to the specified values.
	/// \param triggerPositionPre The TriggerPositionPre value.
	/// \param triggerPositionPost The TriggerPositionPost value.
	BulkSetBuffer2250::BulkSetBuffer2250(uint32_t triggerPositionPre, uint32_t triggerPositionPost) : Helper::DataArray<uint8_t>(12) {
		this->init();
		
		this->setTriggerPositionPre(triggerPositionPre);
		this->setTriggerPositionPost(triggerPositionPost);
	}
	
	/// \brief Get the TriggerPositionPost value.
	/// \return The TriggerPositionPost value.
	uint32_t BulkSetBuffer2250::getTriggerPositionPost() {
		return (uint32_t) this->array[2] | ((uint32_t) this->array[3] << 8) | ((uint32_t) this->array[4] << 16);
	}
	
	/// \brief Set the TriggerPositionPost to the given value.
	/// \param position The new TriggerPositionPost value.
	void BulkSetBuffer2250::setTriggerPositionPost(uint32_t position) {
		this->array[2] = (uint8_t) position;
		this->array[3] = (uint8_t) (position >> 8);
		this->array[4] = (uint8_t) (position >> 16);
	}
	
	/// \brief Get the TriggerPositionPre value.
	/// \return The TriggerPositionPre value.
	uint32_t BulkSetBuffer2250::getTriggerPositionPre() {
		return (uint32_t) this->array[6] | ((uint16_t) this->array[7] << 8) | ((uint16_t) this->array[8] << 16);
	}
	
	/// \brief Set the TriggerPositionPre to the given value.
	/// \param position The new TriggerPositionPre value.
	void BulkSetBuffer2250::setTriggerPositionPre(uint32_t position) {
		this->array[6] = (uint8_t) position;
		this->array[7] = (uint8_t) (position >> 8);
		this->array[8] = (uint8_t) (position >> 16);
	}
	
	/// \brief Initialize the array to the needed values.
	void BulkSetBuffer2250::init() {
		this->array[0] = BULK_FSETBUFFER;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlGetSpeed
	/// \brief Initializes the array.
	ControlGetSpeed::ControlGetSpeed() : Helper::DataArray<uint8_t>(10) {
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
	ControlBeginCommand::ControlBeginCommand(BulkIndex index) : Helper::DataArray<uint8_t>(10) {
		this->init();
		
		this->setIndex(index);
	}
	
	/// \brief Gets the command index.
	/// \return The CommandIndex for the command.
	BulkIndex ControlBeginCommand::getIndex() {
		return (BulkIndex) this->array[1];
	}
	
	/// \brief Sets the command index to the given value.
	/// \param index The new CommandIndex for the command.
	void ControlBeginCommand::setIndex(BulkIndex index) {
		memset(&(this->array[1]), (uint8_t) index, 3);
	}
	
	/// \brief Initialize the array to the needed values.
	void ControlBeginCommand::init() {
		this->array[0] = 0x0f;
	}
	
	
	//////////////////////////////////////////////////////////////////////////////
	// class ControlSetOffset
	/// \brief Sets the data array to the default values.
	ControlSetOffset::ControlSetOffset() : Helper::DataArray<uint8_t>(17) {
	}
	
	/// \brief Sets the offsets to the given values.
	/// \param channel1 The offset for channel 1.
	/// \param channel2 The offset for channel 2.
	/// \param trigger The offset for ext. trigger.
	ControlSetOffset::ControlSetOffset(uint16_t channel1, uint16_t channel2, uint16_t trigger) : Helper::DataArray<uint8_t>(17) {
		this->setChannel(0, channel1);
		this->setChannel(1, channel2);
		this->setTrigger(trigger);
	}
	
	/// \brief Get the offset for the given channel.
	/// \param channel The channel whose offset should be returned.
	/// \return The channel offset value.
	uint16_t ControlSetOffset::getChannel(unsigned int channel) {
		if(channel == 0)
			return ((this->array[0] & 0x0f) << 8) | this->array[1];
		else
			return ((this->array[2] & 0x0f) << 8) | this->array[3];
	}
	
	/// \brief Set the offset for the given channel.
	/// \param channel The channel that should be set.
	/// \param offset The new channel offset value.
	void ControlSetOffset::setChannel(unsigned int channel, uint16_t offset) {
		if(channel == 0) {
			this->array[0] = (uint8_t) (offset >> 8);
			this->array[1] = (uint8_t) offset;
		}
		else {
			this->array[2] = (uint8_t) (offset >> 8);
			this->array[3] = (uint8_t) offset;
		}
	}
	
	/// \brief Get the trigger level.
	/// \return The trigger level value.
	uint16_t ControlSetOffset::getTrigger() {
		return ((this->array[4] & 0x0f) << 8) | this->array[5];
	}
	
	/// \brief Set the trigger level.
	/// \param level The new trigger level value.
	void ControlSetOffset::setTrigger(uint16_t level) {
		this->array[4] = (uint8_t) (level >> 8);
		this->array[5] = (uint8_t) level;
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
	ControlSetRelays::ControlSetRelays(bool ch1Below1V, bool ch1Below100mV, bool ch1CouplingDC, bool ch2Below1V, bool ch2Below100mV, bool ch2CouplingDC, bool triggerExt) : Helper::DataArray<uint8_t>(17) {
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
}
