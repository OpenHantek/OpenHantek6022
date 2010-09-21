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
		this->samplerateChannelMax = 50e6;
		this->samplerateFastMax = 100e6;
		this->samplerateMax = this->samplerateChannelMax;
		this->samplerateDivider = 1;
		this->triggerPosition = 0;
		this->triggerSlope = Dso::SLOPE_POSITIVE;
		this->triggerSpecial = false;
		this->triggerSource = 0;
		this->commandVersion = 0;
		
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
		this->command[COMMAND_SETSAMPLERATE5200] = new CommandSetSamplerate5200();
		this->command[COMMAND_SETBUFFER5200] = new CommandSetBuffer5200();
		this->command[COMMAND_SETTRIGGER5200] = new CommandSetTrigger5200();
		
		for(int command = 0; command < COMMAND_COUNT; command++)
			this->commandPending[command] = false;
		
		// Transmission-ready control commands
		this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
		this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
		this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
		this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;
		
		for(int control = 0; control < CONTROLINDEX_COUNT; control++)
			this->controlPending[control] = false;
		
		// Channel level data
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			for(unsigned int gainId = 0; gainId < GAIN_COUNT; gainId++) {
				this->channelLevels[channel][gainId][OFFSET_START] = 0x0000;
				this->channelLevels[channel][gainId][OFFSET_END] = 0xffff;
			}
		}
		
		// USB device
		this->device = new Device(this);
		
		// Sample buffers
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			this->samples.append(0);
			this->samplesSize.append(0);
		}
		
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
		
		// The control loop is running until the device is disconnected
		int captureState = CAPTURE_WAITING;
		bool samplingStarted = false;
		Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode) -1;
		
		while(captureState != LIBUSB_ERROR_NO_DEVICE && !this->terminate) {
			// Send all pending bulk commands
			for(int command = 0; command < COMMAND_COUNT; command++) {
				if(!this->commandPending[command])
					continue;
				
#ifdef DEBUG
				qDebug("Sending bulk command:%s", Helper::hexDump(this->command[command]->data(), this->command[command]->getSize()).toLocal8Bit().data());
#endif
				
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
				
#ifdef DEBUG
				qDebug("Sending control command 0x%02x:%s", control, Helper::hexDump(this->control[control]->data(), this->control[control]->getSize()).toLocal8Bit().data());
#endif
				
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
			
			if(!this->sampling) {
				samplingStarted = false;
				continue;
			}
			
#ifdef DEBUG
			int lastCaptureState = captureState;
#endif
			captureState = this->getCaptureState();
#ifdef DEBUG
			if(captureState != lastCaptureState)
				qDebug("Capture state changed to %d", captureState);
#endif
			switch(captureState) {
				case CAPTURE_READY:
				case CAPTURE_READY5200:
					// Get data and process it, if we're still sampling
					errorCode = this->getSamples(samplingStarted);
					if(errorCode < 0)
						qDebug("Getting sample data failed: %s", Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
					// Check if we're in single trigger mode
					if(this->triggerMode == Dso::TRIGGERMODE_SINGLE && samplingStarted)
						this->stopSampling();
					
					// Sampling completed, restart it when necessary
					samplingStarted = false;
					
					// Start next capture if necessary by leaving out the break statement
					if(!this->sampling)
						break;
				
				case CAPTURE_WAITING:
					if(samplingStarted && lastTriggerMode == this->triggerMode)
						break;
					
					// Start capturing
					errorCode = this->device->bulkCommand(this->command[COMMAND_STARTSAMPLING]);
					if(errorCode < 0) {
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
#ifdef DEBUG
					qDebug("Starting to capture");
#endif
					
					// Enable trigger
					errorCode = this->device->bulkCommand(this->command[COMMAND_ENABLETRIGGER]);
					if(errorCode < 0) {
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
#ifdef DEBUG
					qDebug("Enabling trigger");
#endif
					
					if(this->triggerMode == Dso::TRIGGERMODE_AUTO) {
						// Force triggering
						errorCode = this->device->bulkCommand(this->command[COMMAND_FORCETRIGGER]);
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
#ifdef DEBUG
					qDebug("Forcing trigger");
#endif
					}
					samplingStarted = true;
					lastTriggerMode = this->triggerMode;
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
	int Control::getSamples(bool process) {
		int errorCode;
		
		// Request data
		errorCode = this->device->bulkCommand(this->command[COMMAND_GETDATA], 1);
		if(errorCode < 0)
			return errorCode;
		
		// Save raw data to temporary buffer
		unsigned int dataCount = this->bufferSize * HANTEK_CHANNELS;
		unsigned int dataLength = dataCount;
		bool using9Bits = false;
		if(this->device->getModel() == MODEL_DSO5200 || this->device->getModel() == MODEL_DSO5200A) {
			using9Bits = true;
			dataLength *= 2;
		}
		
		unsigned char data[dataLength];
		errorCode = this->device->bulkReadMulti(data, dataLength);
		if(errorCode < 0)
			return errorCode;
		
		// Process the data only if we want it
		if(process) {
			// How much data did we really receive?
			dataLength = errorCode;
			if(using9Bits)
				dataCount = dataLength / 2;
			else
				dataCount = dataLength;
			
			this->samplesMutex.lock();
			
			// Convert channel data
			if(((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getFastRate()) {
				// Fast rate mode, one channel is using all buffers
				int channel;
				if(((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getUsedChannels() == USED_CH1)
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
					if(using9Bits) {
						// Additional MSBs after the normal data
						for(unsigned int realPosition = 0; realPosition < dataCount; realPosition++, bufferPosition++) {
							if(bufferPosition >= dataCount)
								bufferPosition %= dataCount;
							
							this->samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition] + ((unsigned short int) data[dataCount + bufferPosition] << 8)) / 0x1ff - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
						}
					}
					else {
						for(unsigned int realPosition = 0; realPosition < dataCount; realPosition++, bufferPosition++) {
							if(bufferPosition >= dataCount)
								bufferPosition %= dataCount;
							
							this->samples[channel][realPosition] = ((double) data[bufferPosition] / 0xff - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
						}
					}
				}
			}
			else {
				// Normal mode, channel are using their separate buffers
				unsigned int channelDataCount = dataCount / HANTEK_CHANNELS;
				unsigned char usedChannels = ((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->getUsedChannels();
				for(int channel = 0; channel < HANTEK_CHANNELS; channel++) {
					if(usedChannels == USED_CH1CH2 || channel == usedChannels) {
						// Reallocate memory for samples if the sample count has changed
						if(!this->samples[channel] || this->samplesSize[channel] != channelDataCount) {
							if(this->samples[channel])
								delete this->samples[channel];
							this->samples[channel] = new double[channelDataCount];
							this->samplesSize[channel] = channelDataCount;
						}
						
						// Convert data from the oscilloscope and write it into the sample buffer
						unsigned int bufferPosition = this->triggerPoint * 2;
						if(using9Bits) {
							// Additional MSBs after the normal data
							for(unsigned int realPosition = 0; realPosition < channelDataCount; realPosition++, bufferPosition += 2) {
								if(bufferPosition >= dataCount)
									bufferPosition %= dataCount;
								
								this->samples[channel][realPosition] = ((double) (data[bufferPosition + HANTEK_CHANNELS - 1 - channel] + (data[dataCount + bufferPosition + HANTEK_CHANNELS - 1 - channel] << 8)) / 0x1ff - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
							}
						}
						else {
							for(unsigned int realPosition = 0; realPosition < channelDataCount; realPosition++, bufferPosition += 2) {
								if(bufferPosition >= dataCount)
									bufferPosition %= dataCount;
								
								this->samples[channel][realPosition] = ((double) data[bufferPosition + HANTEK_CHANNELS - 1 - channel] / 0xff - this->offsetReal[channel]) * this->gainSteps[this->gain[channel]];
							}
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
			emit samplesAvailable(&(this->samples), &(this->samplesSize), (double) this->samplerateMax / this->samplerateDivider, &(this->samplesMutex));
		}
		
		return 0;
	}
	
	/// \brief Sets the size of the sample buffer without updating dependencies.
	/// \param size The buffer size that should be met (S).
	/// \return The buffer size that has been set.
	unsigned long int Control::updateBufferSize(unsigned long int size) {
		BufferSizeId sizeId = (size <= BUFFER_SMALL) ? BUFFERID_SMALL : BUFFERID_LARGE;
		
		switch(this->commandVersion) {
			case 0:
				// SetTriggerAndSamplerate bulk command for buffer size
				((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setBufferSize(sizeId);
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				
				this->bufferSize = (sizeId == BUFFERID_SMALL) ? BUFFER_SMALL : BUFFER_LARGE;
				break;
			
			case 1:
				// SetBuffer5200 bulk command for buffer size
				CommandSetBuffer5200 *commandSetBuffer5200 = (CommandSetBuffer5200 *) this->command[COMMAND_SETBUFFER5200];
				commandSetBuffer5200->setUsedPre(DTRIGGERPOSITION_ON);
				commandSetBuffer5200->setUsedPost(DTRIGGERPOSITION_ON);
				commandSetBuffer5200->setBufferSize(sizeId);
				this->commandPending[COMMAND_SETBUFFER5200] = true;
				
				this->bufferSize = (sizeId == BUFFERID_SMALL) ? BUFFER_SMALL : BUFFER_LARGE5200;
				break;
		}
		
		return this->bufferSize;
	}
	
	/// \brief Try to connect to the oscilloscope.
	void Control::connectDevice() {
		int errorCode;
		
		emit statusMessage(this->device->search(), 0);
		if(!this->device->isConnected())
			return;
		
		// Set all necessary configuration commands as pending
		this->commandPending[COMMAND_SETFILTER] = true;
		this->commandPending[COMMAND_FORCETRIGGER] = false;
		this->commandPending[COMMAND_STARTSAMPLING] = false;
		this->commandPending[COMMAND_ENABLETRIGGER] = false;
		this->commandPending[COMMAND_GETDATA] = false;
		this->commandPending[COMMAND_GETCAPTURESTATE] = false;
		this->commandPending[COMMAND_SETGAIN] = true;
		this->commandPending[COMMAND_SETLOGICALDATA] = false;
		this->commandPending[COMMAND_GETLOGICALDATA] = false;
		
		// Determine the command version we need for this model
		bool unsupported = false;
		switch(this->device->getModel()) {
			case MODEL_DSO2100:
			case MODEL_DSO2150:
				unsupported = true;
			case MODEL_DSO2090:
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				this->commandPending[COMMAND_SETSAMPLERATE5200] = false;
				this->commandPending[COMMAND_SETBUFFER5200] = false;
				this->commandPending[COMMAND_SETTRIGGER5200] = false;
				this->commandVersion = 0;
				break;
			
			case MODEL_DSO2250:
			case MODEL_DSO5200A:
				unsupported = true;
			case MODEL_DSO5200:
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = false;
				this->commandPending[COMMAND_SETSAMPLERATE5200] = true;
				this->commandPending[COMMAND_SETBUFFER5200] = true;
				this->commandPending[COMMAND_SETTRIGGER5200] = true;
				this->commandVersion = 1;
				break;
			
			default:
				this->device->disconnect();
				emit statusMessage(tr("Unknown model"), 0);
				return;
		}
		
		if(unsupported)
			qDebug("Warning: This Hantek DSO model isn't supported officially, so it may not be working as expected. Reports about your experiences are very welcome though (Please open a feature request in the tracker at https://sf.net/projects/openhantek/ or email me directly to oliver.haag@gmail.com). If it's working perfectly I can remove this warning, if not it should be possible to get it working with your help soon.");
		
		for(int control = 0; control < CONTROLINDEX_COUNT; control++)
			this->controlPending[control] = true;
		
		// Maximum possible samplerate for a single channel
		switch(this->device->getModel()) {
			case MODEL_DSO2090:
			case MODEL_DSO2100:
				this->samplerateChannelMax = 50e6;
				this->samplerateFastMax = 100e6;
				break;
			
			case MODEL_DSO2150:
				this->samplerateChannelMax = 50e6;
				this->samplerateFastMax = 150e6;
				break;
			
			default:
				this->samplerateChannelMax = 125e6;
				this->samplerateFastMax = 250e6;
				break;
		}
		this->samplerateMax = this->samplerateChannelMax;
		this->samplerateDivider = 1;
		
		// Get channel level data
		errorCode = this->device->controlRead(CONTROL_VALUE, (unsigned char*) &(this->channelLevels), sizeof(this->channelLevels), (int) VALUE_CHANNELLEVEL);
		if(errorCode < 0) {
			this->device->disconnect();
			emit statusMessage(tr("Couldn't get channel level data from oscilloscope"), 0);
			return;
		}
		
		DsoControl::connectDevice();
	}
	
	/// \brief Sets the size of the oscilloscopes sample buffer.
	/// \param size The buffer size that should be met (S).
	/// \return The buffer size that has been set.
	unsigned long int Control::setBufferSize(unsigned long int size) {
		if(!this->device->isConnected())
			return 0;
		
		this->updateBufferSize(size);
		
		this->setTriggerPosition(this->triggerPosition);
		this->setSamplerate(this->samplerateMax / this->samplerateDivider);
		this->setTriggerSlope(this->triggerSlope);
		
		return this->bufferSize;
	}
	
	/// \brief Sets the samplerate of the oscilloscope.
	/// \param samplerate The samplerate that should be met (S/s).
	/// \return The samplerate that has been set.
	unsigned long int Control::setSamplerate(unsigned long int samplerate) {
		if(!this->device->isConnected() || samplerate == 0)
			return 0;
		
		// Pointers to needed commands
		CommandSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = (CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE];
		CommandSetSamplerate5200 *commandSetSamplerate5200 = (CommandSetSamplerate5200 *) this->command[COMMAND_SETSAMPLERATE5200];
		CommandSetBuffer5200 *commandSetBuffer5200 = (CommandSetBuffer5200 *) this->command[COMMAND_SETBUFFER5200];
		CommandSetTrigger5200 *commandSetTrigger5200 = (CommandSetTrigger5200 *) this->command[COMMAND_SETTRIGGER5200];
		
		// Calculate with fast rate first if only one channel is used
		bool fastRate = false;
		this->samplerateMax = this->samplerateChannelMax;
		if((this->commandVersion == 0) ? (commandSetTriggerAndSamplerate->getUsedChannels() != USED_CH1CH2) : (commandSetTrigger5200->getUsedChannels() != EUSED_CH1CH2)) {
			fastRate = true;
			this->samplerateMax = this->samplerateFastMax;
		}
		
		// The maximum sample rate depends on the buffer size
		unsigned int bufferDivider = 1;
		switch((this->commandVersion == 0) ? commandSetTriggerAndSamplerate->getBufferSize() : commandSetBuffer5200->getBufferSize()) {
			case BUFFERID_ROLL:
				bufferDivider = 1000;
				break;
			case BUFFERID_LARGE:
				bufferDivider = 2;
				break;
			default:
				break;
		}
		
		// Get divider that would provide the requested rate, can't be zero
		this->samplerateMax /= bufferDivider;
		this->samplerateDivider = qMax(this->samplerateMax / samplerate, (long unsigned int) 1);
		
		// Use normal mode if we need valueSlow or it would meet the rate at least as exactly as fast rate mode
		if(fastRate) {
			unsigned long int slowSamplerate = this->samplerateChannelMax / bufferDivider;
			unsigned long int slowDivider = qMax(slowSamplerate / samplerate, (long unsigned int) 1);
			
			if(this->samplerateDivider > 4 || (qAbs((double) slowSamplerate / slowDivider - samplerate) <= qAbs(((double) this->samplerateMax / this->samplerateDivider) - samplerate))) {
				fastRate = false;
				this->samplerateMax = slowSamplerate;
				this->samplerateDivider = slowDivider;
			}
		}
		
		// Split the resulting divider into the values understood by the device
		// The fast value is kept at 4 (or 3) for slow sample rates
		long int valueSlow = qMax(((long int) this->samplerateDivider - 3) / 2, (long int) 0);
		unsigned char valueFast = this->samplerateDivider - valueSlow * 2;
		
		switch(this->commandVersion) {
			case 0:
				// Store samplerate fast value
				commandSetTriggerAndSamplerate->setSamplerateFast(valueFast);
				// Store samplerate slow value (two's complement)
				commandSetTriggerAndSamplerate->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
				// Set fast rate when used
				commandSetTriggerAndSamplerate->setFastRate(fastRate);
				
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
				
			default:
				// Store samplerate fast value
				commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
				// Store samplerate slow value (two's complement)
				commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
				// Set fast rate when used
				commandSetTrigger5200->setFastRate(fastRate);
				
				this->commandPending[COMMAND_SETSAMPLERATE5200] = true;
				this->commandPending[COMMAND_SETTRIGGER5200] = true;
				
				break;
		}
		
		this->updateBufferSize(this->bufferSize);
		this->setTriggerSlope(this->triggerSlope);
		return this->samplerateMax / this->samplerateDivider;
	}	
	
	/// \brief Enables/disables filtering of the given channel.
	/// \param channel The channel that should be set.
	/// \param used true if the channel should be sampled.
	/// \return 0 on success, -1 on invalid channel.
	int Control::setChannelUsed(unsigned int channel, bool used) {
		if(!this->device->isConnected())
			return -2;
		
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
		// SetFilter bulk command for channel filter (used has to be inverted!)
		CommandSetFilter *commandSetFilter = (CommandSetFilter *) this->command[COMMAND_SETFILTER];
		commandSetFilter->setChannel(channel, !used);
		this->commandPending[COMMAND_SETFILTER] = true;
		
		switch(this->commandVersion) {
			case 0: {
				// SetTriggerAndSamplerate bulk command for trigger source
				unsigned char usedChannels = USED_CH1;
				if(!commandSetFilter->getChannel(1)) {
					if(commandSetFilter->getChannel(0))
						usedChannels = USED_CH2;
					else
						usedChannels = USED_CH1CH2;
				}
				((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setUsedChannels(usedChannels);
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				break;
			}
			case 1: {
				unsigned char usedChannels = EUSED_CH1;
				if(!commandSetFilter->getChannel(1)) {
					if(commandSetFilter->getChannel(0))
						usedChannels = EUSED_CH2;
					else
						usedChannels = EUSED_CH1CH2;
				}
				((CommandSetTrigger5200 *) this->command[COMMAND_SETTRIGGER5200])->setUsedChannels(usedChannels);
				this->commandPending[COMMAND_SETTRIGGER5200] = true;
				break;
			}
		}
		
		return 0;
	}
	
	/// \brief Set the coupling for the given channel.
	/// \param channel The channel that should be set.
	/// \param coupling The new coupling for the channel.
	/// \return 0 on success, -1 on invalid channel.
	int Control::setCoupling(unsigned int channel, Dso::Coupling coupling) {
		if(!this->device->isConnected())
			return -2;
		
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
		// SetRelays control command for coupling relays
		((ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS])->setCoupling(channel, coupling != Dso::COUPLING_AC);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		return 0;
	}
	
	/// \brief Sets the gain for the given channel.
	/// \param channel The channel that should be set.
	/// \param gain The gain that should be met (V/div).
	/// \return The gain that has been set, -1 on invalid channel.
	double Control::setGain(unsigned int channel, double gain) {
		if(!this->device->isConnected())
			return -2;
		
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
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
		if(!this->device->isConnected())
			return -2;
		
		if(channel >= HANTEK_CHANNELS)
			return -1;
		
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
		if(!this->device->isConnected())
			return -2;
		
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
		if(!this->device->isConnected())
			return -2;
		
		if((!special && id >= HANTEK_CHANNELS) || (special && id >= HANTEK_SPECIAL_CHANNELS))
			return -1;
		
		// Generate trigger source value that will be transmitted
		int sourceValue;
		if(special)
			sourceValue = TRIGGER_EXT + id;
		else
			sourceValue = TRIGGER_CH1 - id;
		
		switch(this->commandVersion) {
			case 0:
				// SetTriggerAndSamplerate bulk command for trigger source
				((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setTriggerSource(sourceValue);
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				break;
			
			case 1:
				// SetTrigger5200 bulk command for trigger source
				((CommandSetTrigger5200 *) this->command[COMMAND_SETTRIGGER5200])->setTriggerSource(sourceValue);
				this->commandPending[COMMAND_SETTRIGGER5200] = true;
				break;
		}
		
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
		if(!this->device->isConnected())
			return -2;
		
		if(channel >= HANTEK_CHANNELS)
			return -1.0;
		
		// Calculate the trigger level value (0x00 - 0xfe)
		unsigned short int levelValue = (this->offsetReal[channel] + level / this->gainSteps[this->gain[channel]]) * 0xfe + 0.5;
		
		if(!this->triggerSpecial && channel == this->triggerSource) {
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
		if(!this->device->isConnected())
			return -2;
		
		if(slope != Dso::SLOPE_NEGATIVE && slope != Dso::SLOPE_POSITIVE)
			return -1;
		
		switch(this->commandVersion) {
			case 0: {
				// SetTriggerAndSamplerate bulk command for trigger slope
				CommandSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = (CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE];
				
				commandSetTriggerAndSamplerate->setTriggerSlope((/*this->bufferSize != BUFFER_SMALL ||*/ commandSetTriggerAndSamplerate->getSamplerateFast() % 2 == 0) ? slope : Dso::SLOPE_NEGATIVE - slope);
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				break;
			}
			case 1: {
				// SetTrigger5200 bulk command for trigger slope
				((CommandSetTrigger5200 *) this->command[COMMAND_SETTRIGGER5200])->setTriggerSlope(slope);
				this->commandPending[COMMAND_SETTRIGGER5200] = true;
				break;
			}
		}
		
		this->triggerSlope = slope;
		return 0;
	}
	
	/// \brief Set the trigger position.
	/// \param position The new trigger position (in s).
	/// \return The trigger position that has been set.
	double Control::setTriggerPosition(double position) {
		if(!this->device->isConnected())
			return -2;
		
		// All trigger position are measured in samples
		unsigned long int positionSamples = position * this->samplerateMax / this->samplerateDivider;
		
		switch(this->commandVersion) {
			case 0: {
				// Calculate the position value (Start point depending on buffer size)
				unsigned long int positionStart = (this->bufferSize == BUFFER_SMALL) ? 0x77660 : 0x78000;
				
				// SetTriggerAndSamplerate bulk command for trigger position
				((CommandSetTriggerAndSamplerate *) this->command[COMMAND_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(positionStart + positionSamples);
				this->commandPending[COMMAND_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			}
			case 1: {
					// Calculate the position values (Inverse, maximum is 0xffff)
				unsigned short int positionPre = 0xffff - this->bufferSize + positionSamples;
				unsigned short int positionPost = 0xffff - positionSamples;
				
				// SetBuffer5200 bulk command for trigger position
				CommandSetBuffer5200 *commandSetBuffer5200 = (CommandSetBuffer5200 *) this->command[COMMAND_SETBUFFER5200];
				commandSetBuffer5200->setTriggerPositionPre(positionPre);
				commandSetBuffer5200->setTriggerPositionPost(positionPost);
				this->commandPending[COMMAND_SETBUFFER5200] = true;
				
				break;
			}
		}
		
		this->triggerPosition = position;
		return (double) positionSamples / this->samplerateMax * this->samplerateDivider;
	}
	
#ifdef DEBUG
	/// \brief Sends bulk/control commands directly.
	/// \param command The command as string (Has to be parsed).
	/// \return 0 on success, -1 on unknown command, -2 on syntax error.
	int Control::stringCommand(QString command) {
		if(!this->device->isConnected())
			return -3;
		
		QStringList commandParts = command.split(' ', QString::SkipEmptyParts);
		
		if(commandParts.count() >= 1) {
			if(commandParts[0] == "send") {
				if(commandParts.count() >= 2) {
					if(commandParts[1] == "bulk") {
						QString data = command.section(' ', 2, -1, QString::SectionSkipEmpty);
						unsigned char commandCode = 0;
						
						// Read command code (First byte)
						Helper::hexParse(data, &commandCode, 1);
						if(commandCode > COMMAND_COUNT)
							return -2;
						
						// Update bulk command and mark as pending
						Helper::hexParse(data, this->command[commandCode]->data(), this->command[commandCode]->getSize());
						this->commandPending[commandCode] = true;
						return 0;
					}
					else if(commandParts[1] == "control") {
						if(commandParts.count() <= 1)
							return -2;
						
						// Get control code from third part
						unsigned char controlCode = commandParts[2].toUShort();
						int control;
						for(control = 0; control < CONTROLINDEX_COUNT; control++) {
							if(this->controlCode[control] == controlCode)
								break;
						}
						if(control >= CONTROLINDEX_COUNT)
							return -2;
						
						QString data = command.section(' ', 3, -1, QString::SectionSkipEmpty);
						
						// Update control command and mark as pending
						Helper::hexParse(data, this->control[control]->data(), this->control[control]->getSize());
						this->controlPending[control] = true;
						return 0;
					}
				}
			}
		}
		
		return -1;
	}
#endif
}
