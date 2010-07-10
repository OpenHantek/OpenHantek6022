////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/control.cpp
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


#include <QList>
#include <QMutex>


#include "hantek/control.h"

#include "helper.h"
#include "hantek/device.h"
#include "hantek/types.h"


namespace Hantek {
	/// \brief Initializes the command buffers and lists.
	/// \param parent The parent widget.
	Control::Control(QObject *parent) : DsoControl(parent) {
		// Values for the Gain and Timebase enums
		this->gainSteps             << 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00
				<<  8.0 << 16.0 << 40.0;
		this->samplerateSteps                       <<  1e8 <<  5e7 << 25e6 <<  1e7
				<<  5e6 << 25e5 <<  1e6 <<  5e5 << 25e4 <<  1e5 <<  5e4 << 25e3 <<  1e4
				<<  5e3 << 25e2 <<  1e3;
		
		// Special trigger sources
		this->specialTriggerSources << tr("EXT") << tr("EXT/10");
		
		// Transmission-ready bulk commands
		this->command[COMMAND_SETFILTER] = new CommandSetFilter();
		this->command[COMMAND_SETTRIGGERANDSAMPLERATE] = new CommandSetTriggerAndSamplerate();
		this->command[COMMAND_FORCETRIGGER] = new CommandForceTrigger();
		this->command[COMMAND_STARTSAMPLING] = new CommandCaptureStart();
		this->command[COMMAND_ENABLETRIGGER] = new CommandTriggerEnabled();
		this->command[COMMAND_GETDATA] = new CommandGetData();
		this->command[COMMAND_GETCAPTURESTATE] = new CommandGetCaptureState();
		this->command[COMMAND_SETGAIN] = new CommandSetGain();
		this->command[COMMAND_SETLOGICALDATA] = new CommandSetLogicalData();
		this->command[COMMAND_GETLOGICALDATA] = new CommandGetLogicalData();
		
		// Transmission-ready control commands
		this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
		this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
		this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
		this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;
		
		// Channel level data
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			for(unsigned int gainId = 0; gainId < GAIN_COUNT; gainId++) {
				this->channelLevels[channel][gainId][OFFSET_START] = 0x0000;
				this->channelLevels[channel][gainId][OFFSET_END] = 0xffff;
			}
		}
		
		// Cached variables
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			this->setChannelUsed(channel, channel == 0);
			this->setCoupling(channel, Dso::COUPLING_AC);
			this->setGain(channel, 8.0);
			this->setOffset(channel, 0.5);
			this->setTriggerLevel(channel, 0.0);
		}
		this->setSamplerate(1e6);
		this->setBufferSize(BUFFER_SMALL);
		this->setTriggerMode(Dso::TRIGGERMODE_NORMAL);
		this->setTriggerPosition(0.5);
		this->setTriggerSlope(Dso::SLOPE_POSITIVE);
		this->setTriggerSource(false, 0);
		
		// Sample buffers
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			this->samples.append(0);
			this->samplesSize.append(0);
		}
		
		// USB device
		this->device = new Device(this);
		
		connect(this->device, SIGNAL(disconnected()), this, SLOT(disconnectDevice()));
	}
	
	/// \brief Disconnects the device.
	Control::~Control() {
		this->device->disconnect();
	}
	
	/// \brief Gets the physical channel count for this oscilloscope.
	/// \returns The number of physical channels.
	unsigned int Control::getChannelCount() {
		return HANTEK_CHANNELS;
	}
	
	/// \brief Handles all USB things until the device gets disconnected.
	void Control::run() {
		int errorCode;
		
		emit statusMessage(this->device->search(), 0);
		if(!this->device->isConnected())
			return;
		
		// Set all configuration commands as pending
		this->commandPending[COMMAND_SETFILTER] = true;
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
		this->commandPending[COMMAND_FORCETRIGGER] = false;
		this->commandPending[COMMAND_STARTSAMPLING] = false;
		this->commandPending[COMMAND_ENABLETRIGGER] = false;
		this->commandPending[COMMAND_GETDATA] = false;
		this->commandPending[COMMAND_GETCAPTURESTATE] = false;
		this->commandPending[COMMAND_SETGAIN] = true;
		this->commandPending[COMMAND_SETLOGICALDATA] = true;
		this->commandPending[COMMAND_GETLOGICALDATA] = false;
		for(int control = 0; control < CONTROLINDEX_COUNT; control++)
			this->controlPending[control] = true;
		
		// Get calibration data
		errorCode = this->device->controlRead(CONTROL_VALUE, (unsigned char*) &(this->channelLevels), sizeof(this->channelLevels), (int) VALUE_CHANNELLEVEL);
		if(errorCode < 0) {
			this->device->disconnect();
			emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
			return;
		}
		
		// Adapt offsets
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++)
			this->setOffset(channel, this->offset[channel]);
		
		// The control loop is running until the device is disconnected
		int captureState = CAPTURE_WAITING;
		bool samplingStarted = false;
		
		while(captureState != LIBUSB_ERROR_NO_DEVICE && !this->terminate) {
			// Send all pending bulk commands
			for(int command = 0; command < COMMAND_COUNT; command++) {
				if(!this->commandPending[command])
					continue;
				
				errorCode = this->device->bulkCommand(this->command[command]);
				if(errorCode < 0) {
					qDebug("Sending bulk command 0x%02x failed: %s", command, Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
					if(errorCode == LIBUSB_ERROR_NO_DEVICE) {
						captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
				}
				else
					this->commandPending[command] = false;
			}
			if(captureState == LIBUSB_ERROR_NO_DEVICE)
				break;
			
			// Send all pending control commands
			for(int control = 0; control < CONTROLINDEX_COUNT; control++) {
				if(!this->controlPending[control])
					continue;
				
				errorCode = this->device->controlWrite(this->controlCode[control], this->control[control]->data(), this->control[control]->getSize());
				if(errorCode < 0) {
					qDebug("Sending control command 0x%2x failed: %s", control, Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
					if(errorCode == LIBUSB_ERROR_NO_DEVICE) {
						captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
				}
				else
					this->controlPending[control] = false;
			}
			if(captureState == LIBUSB_ERROR_NO_DEVICE)
				break;
			
			// Check the current oscilloscope state every 50 ms
			/// \todo Maybe the time interval could be improved...
			this->msleep(50);
			
			if(!this->sampling)
				continue;
			captureState = this->getCaptureState();
			switch(captureState) {
				case CAPTURE_READY:
				case CAPTURE_READY5200:
					samplingStarted = false;
					
					errorCode = this->getSamples();
					if(errorCode < 0)
						qDebug("Getting sample data failed: %s", Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
					// Start next capture if necessary by leaving out the break statement
					if(!this->sampling)
						break;
				case CAPTURE_WAITING:
					if(samplingStarted)
						break;
					
					// Start capturing
					errorCode = this->device->bulkCommand(this->command[COMMAND_STARTSAMPLING]);
					if(errorCode < 0) {
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
					// Enable trigger
					errorCode = this->device->bulkCommand(this->command[COMMAND_ENABLETRIGGER]);
					if(errorCode < 0) {
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
					if(this->triggerMode == Dso::TRIGGERMODE_AUTO) {
						// Force triggering
						errorCode = this->device->bulkCommand(this->command[COMMAND_FORCETRIGGER]);
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
					}
					samplingStarted = true;
					break;
				case CAPTURE_SAMPLING:
					break;
				default:
					if(captureState < 0)
						qDebug("Getting capture state failed: %s", Helper::libUsbErrorString(captureState).toLocal8Bit().data());
					break;
			}
		}
		
		this->device->disconnect();
		emit statusMessage(tr("The device has been disconnected"), 0);
	}
	
	/// \brief Calculates the trigger point from the CommandGetCaptureState data.
	/// \param value The data value that contains the trigger point.
	/// \return The calculated trigger point for the given data.
	unsigned int Control::calculateTriggerPoint(unsigned int value) {
    unsigned int min = value;
    unsigned int max = 1;
    while(min > 0) {
        min >>= 1;
        max <<= 1;
    }
		max--;
		
    unsigned check = 0;
    unsigned lastLowCheck = 0;
    bool tooHigh = true;
		
    while(max > min) {
			check = (max - min + 1) / 2 + lastLowCheck;
			
			bool higher = check > value;
			if(!higher)
				lastLowCheck = check;
			
			tooHigh = higher == tooHigh;
			if(tooHigh)
				max = (max + min - 1) / 2;
			else
				min = (max + min + 1) / 2;
    }
		
    return min;
	}
	
	/// \brief Gets the current state.
	/// \return The current CaptureState of the oscilloscope.
	int Control::getCaptureState() {
		int errorCode;
		
		errorCode = this->device->bulkCommand(this->command[COMMAND_GETCAPTURESTATE], 1);
		if(errorCode < 0)
			return errorCode;
		
		ResponseGetCaptureState response;
		errorCode = this->device->bulkRead(response.data(), response.getSize());
		if(errorCode < 0)
			return errorCode;
		
		this->triggerPoint = this->calculateTriggerPoint(response.getTriggerPoint());
		
		return (int) response.getCaptureState();
	}
	
	/// \brief Gets sample data from the oscilloscope and converts it.
	/// \return 0 on success, libusb error code on error.
	int Control::getSamples() {
		int errorCode;
		
		errorCode = this->device->bulkCommand(this->command[COMMAND_GETDATA], 1);
		if(errorCode < 0)
			return errorCode;
		
		unsigned int dataCount = this->bufferSize * HANTEK_CHANNELS;
		unsigned char data[dataCount];
		errorCode = this->device->bulkReadMulti(data, dataCount);
		if(errorCode < 0)
			return errorCode;
		
		this->samplesMutex.lock();
		
		// Convert channel data
		if(((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getFastRate()) {
			// Fast rate mode, one channel is using all buffers
			int channel;
			if(((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getUsedChannel() == USED_CH1)
				channel = 0;
			else
				channel = 1;
			
			
			// Clear unused channels
			for(int channelCounter = 0; channelCounter < HANTEK_CHANNELS; channelCounter++)
				if(channelCounter != channel && this->samples[channelCounter]) {
					
					delete this->samples[channelCounter];
					this->samples[channelCounter] = 0;
				}
			
			if(channel < HANTEK_CHANNELS) {
				// Reallocate memory for samples if the sample count has changed
				if(!this->samples[channel] || this->samplesSize[channel] != dataCount) {
					if(this->samples[channel])
						delete this->samples[channel];
					this->samples[channel] = new double[dataCount];
					this->samplesSize[channel] = dataCount;
				}
				
				// Convert data from the oscilloscope and write it into the sample buffer
				unsigned int bufferPosition = this->triggerPoint;
				for(unsigned int realPosition = 0; realPosition < dataCount; realPosition++, bufferPosition++) {
					if(bufferPosition >= dataCount)
						bufferPosition %= dataCount;
					
					this->samples[channel][realPosition] = ((double) data[bufferPosition] / 0xff - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
				}
			}
		}
		else {
			// Normal mode, channel are using their separate buffers
			unsigned char usedChannels = ((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getUsedChannel();
			for(int channel = 0; channel < HANTEK_CHANNELS; channel++) {
				if(usedChannels == USED_CH1CH2 || channel == usedChannels) {
					// Reallocate memory for samples if the sample count has changed
					if(!this->samples[channel] || this->samplesSize[channel] != this->bufferSize) {
						if(this->samples[channel])
							delete this->samples[channel];
						this->samples[channel] = new double[this->bufferSize];
						this->samplesSize[channel] = this->bufferSize;
					}
					
					// Convert data from the oscilloscope and write it into the sample buffer
					unsigned int bufferPosition = this->triggerPoint * 2;
					for(unsigned int realPosition = 0; realPosition < this->bufferSize; realPosition++, bufferPosition += 2) {
						if(bufferPosition >= dataCount)
							bufferPosition %= dataCount;
						
						this->samples[channel][realPosition] = ((double) data[bufferPosition + HANTEK_CHANNELS - 1 - channel] / 256.0 - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
					}
				}
				else if(this->samples[channel]) {
					// Clear unused channels
					delete this->samples[channel];
					this->samples[channel] = 0;
					this->samplesSize[channel] = 0;
				}
			}
		}
		
		this->samplesMutex.unlock();
		emit samplesAvailable(&(this->samples), &(this->samplesSize), this->samplerateSteps[this->samplerate], &(this->samplesMutex));
		
		return 0;
	}
	
	/// \brief Sets the size of the oscilloscopes sample buffer.
	/// \param size The buffer size that should be met (S).
	/// \return The buffer size that has been set.
	double Control::setBufferSize(unsigned int size) {
		unsigned int sizeId = (size <= BUFFER_SMALL) ? 1 : 2;
		
		// SetTriggerAndSamplerate bulk command for samplerate
		((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setSampleSize(sizeId);
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
		
		this->setSamplerate(this->samplerateSteps[this->samplerate]);
		
		this->bufferSize = (sizeId == 1) ? BUFFER_SMALL : BUFFER_LARGE;
		return this->bufferSize;
	}
	
	/// \brief Sets the samplerate of the oscilloscope.
	/// \param samplerate The samplerate that should be met (S/s).
	/// \return The samplerate that has been set.
	unsigned long int Control::setSamplerate(unsigned long int samplerate) {
		// Find lowest supported samplerate thats at least as high as the requested
		int samplerateId;
		for(samplerateId = SAMPLERATE_COUNT - 1; samplerateId > 0; samplerateId--)
			if(this->samplerateSteps[samplerateId] >= samplerate)
				break;
		// Fastrate is only possible if we're not using both channels
		if(samplerateId == SAMPLERATE_100MS && ((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getUsedChannel() == USED_CH1CH2)
			samplerateId = SAMPLERATE_50MS;
		
		// The values that are understood by the oscilloscope
		static const unsigned char valueFastSmall[5] = {0, 1, 2, 3, 4};
		static const unsigned char valueFastLarge[5] = {0, 0, 0, 2, 3};
		static const unsigned short int valueSlowSmall[13] = {0xffff, 0xfffc, 0xfff7, 0xffe8, 0xffce, 0xff9c, 0xff07, 0xfe0d, 0xfc19, 0xf63d, 0xec79, 0xd8f1, 0xffed};
		static const unsigned short int valueSlowLarge[13] = {0xffff, 0x0000, 0xfffc, 0xfff7, 0xffe8, 0xffce, 0xff9d, 0xff07, 0xfe0d, 0xfc19, 0xf63d, 0xec79, 0xffed}; /// \todo Check those values
		
		// SetTriggerAndSamplerate bulk command for samplerate
		CommandSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = (CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE];
		
		// Set SamplerateFast bits for high sampling rates
		if(samplerateId <= SAMPLERATE_5MS)
			commandSetTriggerAndSamplerate->setSamplerateFast(this->bufferSize == BUFFER_SMALL ? valueFastSmall[samplerateId] : valueFastLarge[samplerateId]);
		else
			commandSetTriggerAndSamplerate->setSamplerateFast(4);
		
		// Set normal Samplerate value for lower sampling rates
		if(samplerateId >= SAMPLERATE_2_5MS)
			commandSetTriggerAndSamplerate->setSamplerate(this->bufferSize == BUFFER_SMALL ? valueSlowSmall[samplerateId - SAMPLERATE_2_5MS] : valueSlowLarge[samplerateId - SAMPLERATE_2_5MS]);
		else
			commandSetTriggerAndSamplerate->setSamplerate(0x0000);
		
		// Set fast rate when used
		commandSetTriggerAndSamplerate->setFastRate(samplerateId == SAMPLERATE_100MS);
		
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
		
		this->samplerate = (Samplerate) samplerateId;
		return this->samplerateSteps[samplerateId];
	}	
	
	/// \brief Enables/disables filtering of the given channel.
	/// \param channel The channel that should be set.
	/// \param filtered true if the channel should be filtered.
	/// \return 0 on success, -1 on invalid channel.
	int Control::setChannelUsed(unsigned int channel, bool used) {
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
		// SetFilter bulk command for channel filter (used has to be inverted!)
		CommandSetFilter *commandSetFilter = (CommandSetFilter *) this->command[COMMAND_SETFILTER];
		commandSetFilter->setChannel(channel, !used);
		this->commandPending[COMMAND_SETFILTER] = true;
		
		// SetTriggerAndSamplerate bulk command for trigger source
		unsigned char usedChannel = USED_CH1;
		if(!commandSetFilter->getChannel(1)) {
			if(commandSetFilter->getChannel(0))
				usedChannel = USED_CH2;
			else
				usedChannel = USED_CH1CH2;
		}
		((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setUsedChannel(usedChannel);
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;		
		
		return 0;
	}
	
	/// \brief Set the coupling for the given channel.
	/// \param channel The channel that should be set.
	/// \param coupling The new coupling for the channel.
	/// \return 0 on success, -1 on invalid channel.
	int Control::setCoupling(unsigned int channel, Dso::Coupling coupling) {
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
		// SetRelays control command for coupling relays
		((ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS])->setCoupling(channel, coupling != Dso::COUPLING_AC);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		return 0;
	}
	
	/// \brief Sets the gain for the given channel.
	/// \param gain The gain that should be met (V/div).
	/// \return The gain that has been set, -1.0 on invalid channel.
	double Control::setGain(unsigned int channel, double gain) {
		if(channel >= HANTEK_CHANNELS)
			return -1.0;
		
		// Find lowest gain voltage thats at least as high as the requested
		int gainId;
		for(gainId = 0; gainId < GAIN_COUNT - 1; gainId++)
			if(this->gainSteps[gainId] >= gain)
				break;
		
		// SetGain bulk command for gain
		((CommandSetGain *) this->command[COMMAND_SETGAIN])->setGain(channel, gainId % 3);
		this->commandPending[COMMAND_SETGAIN] = true;
		
		// SetRelays control command for gain relays
		ControlSetRelays *controlSetRelays = (ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS];
		controlSetRelays->setBelow1V(channel, gainId < GAIN_1V);
		controlSetRelays->setBelow100mV(channel, gainId < GAIN_100MV);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		this->gain[channel] = (Gain) gainId;
		
		this->setOffset(channel, this->offset[channel]);
		
		return this->gainSteps[gainId];
	}
	
	/// \brief Set the offset for the given channel.
	/// \param channel The channel that should be set.
	/// \param offset The new offset value (0.0 - 1.0).
	/// \return The offset that has been set, -1.0 on invalid channel.
	double Control::setOffset(unsigned int channel, double offset) {
		if(channel >= HANTEK_CHANNELS)
			return -1.0;
		
		// Calculate the offset value (The range is given by the calibration data)
		unsigned short int minimum = this->channelLevels[channel][this->gain[channel]][OFFSET_START] >> 8;
		unsigned short int maximum = this->channelLevels[channel][this->gain[channel]][OFFSET_END] >> 8;
		unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
		double offsetReal = (double) (offsetValue - minimum) / (maximum - minimum);
		
		// SetOffset control command for channel offset
		((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setChannel(channel, offsetValue);
		this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		
		this->offset[channel] = offset;
		this->offsetReal[channel] = offsetReal;
		
		this->setTriggerLevel(channel, this->triggerLevel[channel]);
		
		return offsetReal;
	}
	
	/// \brief Set the trigger mode.
	/// \return 0 on success, -1 on invalid mode.
	int Control::setTriggerMode(Dso::TriggerMode mode) {
		if(mode < Dso::TRIGGERMODE_AUTO || mode > Dso::TRIGGERMODE_SINGLE)
			return -1;
		
		this->triggerMode = mode;
		return 0;
	}
	
	/// \brief Set the trigger source.
	/// \param special true for a special channel (EXT, ...) as trigger source.
	/// \param id The number of the channel, that should be used as trigger.
	/// \return 0 on success, -1 on invalid channel.
	int Control::setTriggerSource(bool special, unsigned int id) {
		if((!special && id >= HANTEK_CHANNELS) || (special && id >= HANTEK_SPECIAL_CHANNELS))
			return -1;
		
		// Generate trigger source value that will be transmitted
		int sourceValue;
		if(special)
			sourceValue = TRIGGER_EXT + id;
		else
			sourceValue = TRIGGER_CH1 - id;
		
		// SetTriggerAndSamplerate bulk command for trigger source
		((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setTriggerSource(sourceValue);
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;		
		
		// SetRelays control command for external trigger relay
		((ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS])->setTrigger(special);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		this->triggerSpecial = special;
		this->triggerSource = id;
		
		// Apply trigger level of the new source
		if(special) {
			// SetOffset control command for changed trigger level
			((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setTrigger(0x7f);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		else
			this->setTriggerLevel(id, this->triggerLevel[id]);
		
		return 0;
	}
	
	/// \brief Set the trigger level.
	/// \param channel The channel that should be set.
	/// \param level The new trigger level (V).
	/// \return The trigger level that has been set, -1.0 on invalid channel.
	double Control::setTriggerLevel(unsigned int channel, double level) {
		if(channel >= HANTEK_CHANNELS)
			return -1.0;
		
		// Calculate the trigger level value (0x00 - 0xfe)
		unsigned short int levelValue = (this->offsetReal[channel] + level / this->gainSteps[this->gain[channel]]) * 0xfe + 0.5;
		
		if(this->triggerSpecial && channel == this->triggerSource) {
			// SetOffset control command for trigger level
			((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setTrigger(levelValue);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		
		/// \todo Get alternating trigger in here
		
		this->triggerLevel[channel] = level;
		return (double) (levelValue / 0xfe - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
	}
	
	/// \brief Set the trigger slope.
	/// \param slope The Slope that should cause a trigger.
	/// \return 0 on success, -1 on invalid slope.
	int Control::setTriggerSlope(Dso::Slope slope) {
		if(slope != Dso::SLOPE_NEGATIVE && slope != Dso::SLOPE_POSITIVE)
			return -1;
		
		// SetTriggerAndSamplerate bulk command for trigger position
		((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setTriggerSlope(slope);
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
		
		return 0;
	}
	
	/// \brief Set the trigger position.
	/// \param level The new trigger position (0.0 - 1.0).
	/// \return The trigger position that has been set.
	double Control::setTriggerPosition(double position) {
		// Calculate the position value (Varying start point, measured in samples)
		//unsigned long int positionRange = (this->bufferSize == BUFFER_SMALL) ? 10000 : 32768;
		unsigned long int positionStart = (this->bufferSize == BUFFER_SMALL) ? 0x77660 : 0x78000;
		unsigned long int positionValue = position * this->samplerateSteps[this->samplerate] + positionStart;
		
		// SetTriggerAndSamplerate bulk command for trigger position
		((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(positionValue);
		this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
		
		return (double) (positionValue - positionStart) / this->samplerateSteps[this->samplerate];
	}
}
