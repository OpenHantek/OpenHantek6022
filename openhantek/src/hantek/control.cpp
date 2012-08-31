////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/control.cpp
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010, 2011  Oliver Haag
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


#include <limits>

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
		// Use DSO-2090 specification as default
		this->specification.command.bulk.setBuffer = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.bulk.setFilter = BULK_SETFILTER;
		this->specification.command.bulk.setGain = BULK_SETGAIN;
		this->specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.control.setOffset = CONTROL_SETOFFSET;
		this->specification.command.control.setRelays = CONTROL_SETRELAYS;
		this->specification.command.values.offsetLimits = VALUE_OFFSETLIMITS;
		this->specification.command.values.voltageLimits = (ControlValue) -1;
		
		this->specification.samplerate.single.base = 50e6;
		this->specification.samplerate.single.max = 50e6;
		this->specification.samplerate.multi.base = 100e6;
		this->specification.samplerate.multi.max = 100e6;
		
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; channel++) {
			for(unsigned int gainId = 0; gainId < 9; gainId++) {
				this->specification.offsetLimit[channel][gainId][OFFSET_START] = 0x0000;
				this->specification.offsetLimit[channel][gainId][OFFSET_END] = 0xffff;
			}
		}
		
		// Set settings to default values
		this->settings.bufferSizeId = 0;
		this->settings.samplerate.limits = &(this->specification.samplerate.single);
		this->settings.samplerate.downsampling = 1;
		this->settings.trigger.position = 0;
		this->settings.trigger.slope = Dso::SLOPE_POSITIVE;
		this->settings.trigger.special = false;
		this->settings.trigger.source = 0;
		
		// Special trigger sources
		this->specialTriggerSources << tr("EXT") << tr("EXT/10");
		
		// Transmission-ready bulk commands
		this->command[BULK_SETFILTER] = new BulkSetFilter();
		this->command[BULK_SETTRIGGERANDSAMPLERATE] = new BulkSetTriggerAndSamplerate();
		this->command[BULK_FORCETRIGGER] = new BulkForceTrigger();
		this->command[BULK_STARTSAMPLING] = new BulkCaptureStart();
		this->command[BULK_ENABLETRIGGER] = new BulkTriggerEnabled();
		this->command[BULK_GETDATA] = new BulkGetData();
		this->command[BULK_GETCAPTURESTATE] = new BulkGetCaptureState();
		this->command[BULK_SETGAIN] = new BulkSetGain();
		this->command[BULK_SETLOGICALDATA] = new BulkSetLogicalData();
		this->command[BULK_GETLOGICALDATA] = new BulkGetLogicalData();
		this->command[BULK_SETSAMPLERATE5200] = new BulkSetSamplerate5200();
		this->command[BULK_SETBUFFER5200] = new BulkSetBuffer5200();
		this->command[BULK_SETTRIGGER5200] = new BulkSetTrigger5200();
		
		for(int command = 0; command < BULK_COUNT; command++)
			this->commandPending[command] = false;
		
		// Transmission-ready control commands
		this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
		this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
		this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
		this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;
		
		for(int control = 0; control < CONTROLINDEX_COUNT; control++)
			this->controlPending[control] = false;
		
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
		int errorCode, cycleCounter = 0, startCycle = 0;
		
		// The control loop is running until the device is disconnected
		int captureState = CAPTURE_WAITING;
		bool samplingStarted = false;
		Dso::TriggerMode lastTriggerMode = (Dso::TriggerMode) -1;
		
		while(captureState != LIBUSB_ERROR_NO_DEVICE && !this->terminate) {
			// Send all pending bulk commands
			for(int command = 0; command < BULK_COUNT; command++) {
				if(!this->commandPending[command])
					continue;
				
#ifdef DEBUG
				qDebug("Sending bulk command:%s", Helper::hexDump(this->command[command]->data(), this->command[command]->getSize()).toLocal8Bit().data());
#endif
				
				errorCode = this->device->bulkCommand(this->command[command]);
				if(errorCode < 0) {
					qWarning("Sending bulk command 0x%02x failed: %s", command, Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
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
					qWarning("Sending control command 0x%2x failed: %s", control, Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
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
			
			// Check the current oscilloscope state everytime 25% of the time the buffer should be refilled
			// Not more often than every 10 ms though
			int cycleTime = qMax((unsigned long int) (this->specification.bufferSizes[this->settings.bufferSizeId] / this->settings.samplerate.current * 250), (long unsigned int) 10);
			this->msleep(cycleTime);
			
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
						qWarning("Getting sample data failed: %s", Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
					// Check if we're in single trigger mode
					if(this->settings.trigger.mode == Dso::TRIGGERMODE_SINGLE && samplingStarted)
						this->stopSampling();
					
					// Sampling completed, restart it when necessary
					samplingStarted = false;
					
					// Start next capture if necessary by leaving out the break statement
					if(!this->sampling)
						break;
				
				case CAPTURE_WAITING:
					if(samplingStarted && lastTriggerMode == this->settings.trigger.mode) {
						cycleCounter++;
						
						if(cycleCounter == startCycle) {
							// Buffer refilled completely since start of sampling, enable the trigger now
							errorCode = this->device->bulkCommand(this->command[BULK_ENABLETRIGGER]);
							if(errorCode < 0) {
								if(errorCode == LIBUSB_ERROR_NO_DEVICE)
									captureState = LIBUSB_ERROR_NO_DEVICE;
								break;
							}
#ifdef DEBUG
							qDebug("Enabling trigger");
#endif
						}
						else if(cycleCounter >= 8 + startCycle && this->settings.trigger.mode == Dso::TRIGGERMODE_AUTO) {
							// Force triggering
							errorCode = this->device->bulkCommand(this->command[BULK_FORCETRIGGER]);
							if(errorCode == LIBUSB_ERROR_NO_DEVICE)
								captureState = LIBUSB_ERROR_NO_DEVICE;
#ifdef DEBUG
							qDebug("Forcing trigger");
#endif
						}
						
						if(cycleCounter < 50 || cycleCounter < 4000 / cycleTime)
							break;
					}
					
					// Start capturing
					errorCode = this->device->bulkCommand(this->command[BULK_STARTSAMPLING]);
					if(errorCode < 0) {
						if(errorCode == LIBUSB_ERROR_NO_DEVICE)
							captureState = LIBUSB_ERROR_NO_DEVICE;
						break;
					}
#ifdef DEBUG
					qDebug("Starting to capture");
#endif
					
					samplingStarted = true;
					cycleCounter = 0;
					startCycle = this->settings.trigger.position * 1000 / cycleTime + 1;
					lastTriggerMode = this->settings.trigger.mode;
					break;
				
				case CAPTURE_SAMPLING:
					break;
				default:
					if(captureState < 0)
						qWarning("Getting capture state failed: %s", Helper::libUsbErrorString(captureState).toLocal8Bit().data());
					break;
			}
		}
		
		this->device->disconnect();
		emit statusMessage(tr("The device has been disconnected"), 0);
	}
	
	/// \brief Calculates the trigger point from the CommandGetCaptureState data.
	/// \param value The data value that contains the trigger point.
	/// \return The calculated trigger point for the given data.
	unsigned short int Control::calculateTriggerPoint(unsigned short int value) {
		unsigned short int result = value;

		// Each set bit inverts all bits with a lower value
		for(unsigned short int bitValue = 1; bitValue; bitValue <<= 1)
			if(result & bitValue)
				result ^= bitValue - 1;

		return result;
	}
	
	/// \brief Gets the current state.
	/// \return The current CaptureState of the oscilloscope.
	int Control::getCaptureState() {
		int errorCode;
		
		errorCode = this->device->bulkCommand(this->command[BULK_GETCAPTURESTATE], 1);
		if(errorCode < 0)
			return errorCode;
		
		BulkResponseGetCaptureState response;
		errorCode = this->device->bulkRead(response.data(), response.getSize());
		if(errorCode < 0)
			return errorCode;
		
		this->settings.trigger.point = this->calculateTriggerPoint(response.getTriggerPoint());
		
		return (int) response.getCaptureState();
	}
	
	/// \brief Gets sample data from the oscilloscope and converts it.
	/// \return 0 on success, libusb error code on error.
	int Control::getSamples(bool process) {
		int errorCode;
		
		// Request data
		errorCode = this->device->bulkCommand(this->command[BULK_GETDATA], 1);
		if(errorCode < 0)
			return errorCode;
		
		// Save raw data to temporary buffer
		unsigned int dataCount = this->specification.bufferSizes[this->settings.bufferSizeId] * HANTEK_CHANNELS;
		unsigned int dataLength = dataCount;
		bool using10Bits = false;
		if(this->device->getModel() == MODEL_DSO5200 || this->device->getModel() == MODEL_DSO5200A) {
			using10Bits = true;
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
			if(using10Bits)
				dataCount = dataLength / 2;
			else
				dataCount = dataLength;
			
			this->samplesMutex.lock();
			
			// Get oscilloscope settings
			bool fastRate;
			UsedChannels usedChannels;
			if(this->specification.command.bulk.setTrigger == BULK_SETTRIGGERANDSAMPLERATE) {
				fastRate = ((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->getFastRate();
				usedChannels = (UsedChannels) ((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->getUsedChannels();
			}
			else {
				fastRate = ((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->getFastRate();
				usedChannels = (UsedChannels) ((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->getUsedChannels();
			}
			// Convert channel data
			if(fastRate) {
				// Fast rate mode, one channel is using all buffers
				int channel;
				if(usedChannels == USED_CH1)
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
					unsigned int bufferPosition = (this->settings.trigger.point + 1) * 2;
					if(using10Bits) {
						// Additional 2 most significant bits after the normal data
						unsigned int extraBitsPosition; // Track the position of the extra bits in the additional byte
						
						for(unsigned int realPosition = 0; realPosition < dataCount; realPosition++, bufferPosition++) {
							if(bufferPosition >= dataCount)
								bufferPosition %= dataCount;
							
							extraBitsPosition = bufferPosition % HANTEK_CHANNELS;
							
							this->samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition] + (((unsigned short int) data[dataCount + bufferPosition - extraBitsPosition] << (8 - (HANTEK_CHANNELS - 1 - extraBitsPosition) * 2)) & 0x0200)) / this->specification.voltageLimit[HANTEK_CHANNELS - 1 - extraBitsPosition][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
						}
					}
					else {
						for(unsigned int realPosition = 0; realPosition < dataCount; realPosition++, bufferPosition++) {
							if(bufferPosition >= dataCount)
								bufferPosition %= dataCount;
							
							this->samples[channel][realPosition] = ((double) data[bufferPosition] / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
						}
					}
				}
			}
			else {
				// Normal mode, channels are using their separate buffers
				unsigned int channelDataCount = dataCount / HANTEK_CHANNELS;
				
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
						unsigned int bufferPosition = (this->settings.trigger.point + 1) * 2;
						if(using10Bits) {
							// Additional 2 most significant bits after the normal data
							for(unsigned int realPosition = 0; realPosition < channelDataCount; realPosition++, bufferPosition += 2) {
								if(bufferPosition >= dataCount)
									bufferPosition %= dataCount;
								
								this->samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition + HANTEK_CHANNELS - 1 - channel] + (((unsigned short int) data[dataCount + bufferPosition] << (8 - channel * 2)) & 0x0200)) / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
							}
						}
						else {
							for(unsigned int realPosition = 0; realPosition < channelDataCount; realPosition++, bufferPosition += 2) {
								if(bufferPosition >= dataCount)
									bufferPosition %= dataCount;
								
								this->samples[channel][realPosition] = ((double) data[bufferPosition + HANTEK_CHANNELS - 1 - channel] / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
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
			emit samplesAvailable(&(this->samples), &(this->samplesSize), this->settings.samplerate.current, &(this->samplesMutex));
		}
		
		return 0;
	}
	
	/// \brief Sets the size of the sample buffer without updating dependencies.
	/// \param size The buffer size that should be met (S).
	/// \return The buffer size that has been set, 0 on error.
	unsigned long int Control::updateBufferSize(unsigned long int size) {
		// Get the buffer size supporting the highest samplerate while meeting the requirement
		int bestSizeId = -1;
		for(int sizeId = 0; sizeId < this->specification.bufferSizes.count(); sizeId++) {
			if(this->specification.bufferSizes[sizeId] >= size) {
				// We meet the size-requirement, check if we provide the highest possible samplerate
				if(bestSizeId == -1 || this->specification.bufferSizes[bestSizeId] < size || this->specification.bufferDividers[sizeId] < this->specification.bufferDividers[bestSizeId])
					bestSizeId = sizeId;
			}
			else {
				// We don't meet the size-requirement, but maybe we're still the one coming closest
				if(bestSizeId == -1 || this->specification.bufferSizes[sizeId] > this->specification.bufferSizes[bestSizeId])
					bestSizeId = sizeId;
			}
		}
		
		switch(this->specification.command.bulk.setBuffer) {
			case BULK_SETTRIGGERANDSAMPLERATE:
				// SetTriggerAndSamplerate bulk command for buffer size
				((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->setBufferSize(bestSizeId);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			
			case BULK_SETBUFFER5200: {
				// SetBuffer5200 bulk command for buffer size
				BulkSetBuffer5200 *commandSetBuffer5200 = (BulkSetBuffer5200 *) this->command[BULK_SETBUFFER5200];
				commandSetBuffer5200->setUsedPre(DTRIGGERPOSITION_ON);
				commandSetBuffer5200->setUsedPost(DTRIGGERPOSITION_ON);
				commandSetBuffer5200->setBufferSize(bestSizeId);
				this->commandPending[BULK_SETBUFFER5200] = true;
				
				break;
			}
			
			default:
				return 0;
		}
		
		this->settings.bufferSizeId = bestSizeId;
		
		return this->specification.bufferSizes[this->settings.bufferSizeId];
	}
	
	/// \brief Try to connect to the oscilloscope.
	void Control::connectDevice() {
		int errorCode;
		
		emit statusMessage(this->device->search(), 0);
		if(!this->device->isConnected())
			return;
		
		// Initialize the commands used on the DSO-2090 as pending
		this->commandPending[BULK_SETFILTER] = true;
		this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
		this->commandPending[BULK_FORCETRIGGER] = false;
		this->commandPending[BULK_STARTSAMPLING] = false;
		this->commandPending[BULK_ENABLETRIGGER] = false;
		this->commandPending[BULK_GETDATA] = false;
		this->commandPending[BULK_GETCAPTURESTATE] = false;
		this->commandPending[BULK_SETGAIN] = true;
		this->commandPending[BULK_SETLOGICALDATA] = false;
		this->commandPending[BULK_GETLOGICALDATA] = false;
		this->commandPending[BULK_UNKNOWN_0A] = false;
		this->commandPending[BULK_UNKNOWN_0B] = false;
		this->commandPending[BULK_SETSAMPLERATE5200] = false;
		this->commandPending[BULK_SETBUFFER5200] = false;
		this->commandPending[BULK_SETTRIGGER5200] = false;
		// Initialize the command versions to the ones used on the DSO-2090
		this->specification.command.bulk.setBuffer = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.bulk.setFilter = BULK_SETFILTER;
		this->specification.command.bulk.setGain = BULK_SETGAIN;
		this->specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
		this->specification.command.control.setOffset = CONTROL_SETOFFSET;
		this->specification.command.control.setRelays = CONTROL_SETRELAYS;
		this->specification.command.values.offsetLimits = VALUE_OFFSETLIMITS;
		this->specification.command.values.voltageLimits = (ControlValue) -1;
		
		// Determine the command version we need for this model
		bool unsupported = false;
		switch(this->device->getModel()) {
			case MODEL_DSO2150:
				unsupported = true;
			
			case MODEL_DSO2090:
				// Keep the defaults we've set before
				break;
			
			case MODEL_DSO2250:
			case MODEL_DSO5200A:
				unsupported = true;
			
			case MODEL_DSO5200:
				this->specification.command.bulk.setBuffer = BULK_SETBUFFER5200;
				this->specification.command.bulk.setSamplerate = BULK_SETSAMPLERATE5200;
				this->specification.command.bulk.setTrigger = BULK_SETTRIGGER5200;
				this->specification.command.values.voltageLimits = VALUE_VOLTAGELIMITS;
				
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = false;
				this->commandPending[BULK_SETSAMPLERATE5200] = true;
				this->commandPending[BULK_SETBUFFER5200] = true;
				this->commandPending[BULK_SETTRIGGER5200] = true;
				
				break;
			
			default:
				this->device->disconnect();
				emit statusMessage(tr("Unknown model"), 0);
				return;
		}
		
		if(unsupported)
			qWarning("Warning: This Hantek DSO model isn't supported officially, so it may not be working as expected. Reports about your experiences are very welcome though (Please open a feature request in the tracker at https://sf.net/projects/openhantek/ or email me directly to oliver.haag@gmail.com). If it's working perfectly I can remove this warning, if not it should be possible to get it working with your help soon.");
		
		for(int control = 0; control < CONTROLINDEX_COUNT; control++)
			this->controlPending[control] = true;
		
		// Maximum possible samplerate for a single channel and dividers for buffer sizes
		this->specification.bufferDividers.clear();
		this->specification.bufferSizes.clear();
		this->specification.gainSteps.clear();
		for(int channel = 0; channel < HANTEK_CHANNELS; channel++)
			this->specification.voltageLimit[channel].clear();
		
		switch(this->device->getModel()) {
			case MODEL_DSO2250:
			case MODEL_DSO5200:
			case MODEL_DSO5200A:
				this->specification.samplerate.single.base = 100e6;
				this->specification.samplerate.single.max = 125e6;
				this->specification.samplerate.multi.base = 200e6;
				this->specification.samplerate.multi.max = 250e6;
				this->specification.bufferDividers << 1000 << 1 << 2;
				this->specification.bufferSizes << ULONG_MAX << 10240 << 14336;
				this->specification.gainSteps
					<< 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0 << 80.0;
				/// \todo Use calibration data to get the DSO-5200(A) sample ranges
				for(int channel = 0; channel < HANTEK_CHANNELS; channel++)
					this->specification.voltageLimit[channel]
					<<  368 <<  454 <<  908 <<  368 <<  454 <<  908 <<  368 <<  454 <<  908;
				this->specification.gainIndex
					<<    1 <<    0 <<    0 <<    1 <<    0 <<    0 <<    1 <<    0 <<    0;
				break;
			
			case MODEL_DSO2150:
				this->specification.samplerate.single.base = 50e6;
				this->specification.samplerate.single.max = 75e6;
				this->specification.samplerate.multi.base = 100e6;
				this->specification.samplerate.multi.max = 150e6;
				this->specification.bufferDividers << 1000 << 1 << 2;
				this->specification.bufferSizes << ULONG_MAX << 10240 << 32768;
				this->specification.gainSteps
					<< 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
				for(int channel = 0; channel < HANTEK_CHANNELS; channel++)
					this->specification.voltageLimit[channel]
					<<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
				this->specification.gainIndex
					<<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;
				break;
			
			default:
				this->specification.samplerate.single.base = 50e6;
				this->specification.samplerate.single.max = 50e6;
				this->specification.samplerate.multi.base = 100e6;
				this->specification.samplerate.multi.max = 100e6;
				this->specification.bufferDividers << 1000 << 1 << 2;
				this->specification.bufferSizes << ULONG_MAX << 10240 << 32768;
				this->specification.gainSteps
					<< 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
				for(int channel = 0; channel < HANTEK_CHANNELS; channel++)
					this->specification.voltageLimit[channel]
					<<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
				this->specification.gainIndex
					<<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;
				break;
		}
		this->settings.samplerate.limits = &(this->specification.samplerate.single);
		this->settings.samplerate.downsampling = 1;
		
		// Get channel level data
		errorCode = this->device->controlRead(CONTROL_VALUE, (unsigned char *) &(this->specification.offsetLimit), sizeof(this->specification.offsetLimit), (int) VALUE_OFFSETLIMITS);
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
		
		this->setTriggerPosition(this->settings.trigger.position);
		this->setSamplerate();
		
		return this->specification.bufferSizes[this->settings.bufferSizeId];
	}
	
	/// \brief Sets the samplerate of the oscilloscope.
	/// \param samplerate The samplerate that should be met (S/s).
	/// \return The samplerate that has been set.
	unsigned long int Control::setSamplerate(unsigned long int samplerate) {
		if(!this->device->isConnected())
			return 0;
		
		// Keep samplerate if no parameter was given
		if(!samplerate)
			samplerate = this->settings.samplerate.current;
		// Abort samplerate calculation if we didn't get a valid value yet
		if(!samplerate)
			return samplerate;
		
		// Calculate with fast rate first if only one channel is used
		bool fastRate = false;
		this->settings.samplerate.limits = &(this->specification.samplerate.single);
		if(this->settings.usedChannels <= 1) {
			fastRate = true;
			this->settings.samplerate.limits = &(this->specification.samplerate.multi);
		}
		
		// Get downsampling factor that would provide the requested rate
		this->settings.samplerate.downsampling = this->settings.samplerate.limits->base / this->specification.bufferDividers[this->settings.bufferSizeId] / samplerate;
		// A downsampling factor of zero will result in the maximum rate
		if(this->settings.samplerate.downsampling)
			this->settings.samplerate.current = this->settings.samplerate.limits->base / this->specification.bufferDividers[this->settings.bufferSizeId] / this->settings.samplerate.downsampling;
		else
			this->settings.samplerate.current = this->settings.samplerate.limits->max / this->specification.bufferDividers[this->settings.bufferSizeId];
		
		// Maybe normal mode would be sufficient or even better than fast rate mode
		if(fastRate) {
			// Don't set the downsampling factor to zero (maximum rate) if we could use fast rate mode anyway
			unsigned long int slowDownsampling = qMax(this->specification.samplerate.single.base / this->specification.bufferDividers[this->settings.bufferSizeId] / samplerate, (long unsigned int) 1);
			
			// Use normal mode if we need valueSlow or it would meet the rate at least as exactly as fast rate mode
			if(this->settings.samplerate.downsampling > 4 || (qAbs((double) this->specification.samplerate.single.base / this->specification.bufferDividers[this->settings.bufferSizeId] / slowDownsampling - samplerate) <= qAbs(this->settings.samplerate.current - samplerate))) {
				fastRate = false;
				this->settings.samplerate.limits = &(this->specification.samplerate.single);
				this->settings.samplerate.downsampling = slowDownsampling;
				this->settings.samplerate.current = this->specification.samplerate.single.base / this->specification.bufferDividers[this->settings.bufferSizeId] / this->settings.samplerate.downsampling;
			}
		}
		
		// Split the resulting divider into the values understood by the device
		// The fast value is kept at 4 (or 3) for slow sample rates
		long int valueSlow = qMax(((long int) this->settings.samplerate.downsampling - 3) / 2, (long int) 0);
		unsigned char valueFast = this->settings.samplerate.downsampling - valueSlow * 2;
		
		switch(this->specification.command.bulk.setSamplerate) {
			case BULK_SETTRIGGERANDSAMPLERATE: {
				// Pointers to needed commands
				BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = (BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE];
				
				// Store samplerate fast value
				commandSetTriggerAndSamplerate->setSamplerateFast(valueFast);
				// Store samplerate slow value (two's complement)
				commandSetTriggerAndSamplerate->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
				// Set fast rate when used
				commandSetTriggerAndSamplerate->setFastRate(fastRate);
				
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			}
			case BULK_SETSAMPLERATE5200: {
				// Pointers to needed commands
				BulkSetSamplerate5200 *commandSetSamplerate5200 = (BulkSetSamplerate5200 *) this->command[BULK_SETSAMPLERATE5200];
				BulkSetTrigger5200 *commandSetTrigger5200 = (BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200];
				// Store samplerate fast value
				commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
				// Store samplerate slow value (two's complement)
				commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
				// Set fast rate when used
				commandSetTrigger5200->setFastRate(fastRate);
				
				this->commandPending[BULK_SETSAMPLERATE5200] = true;
				this->commandPending[BULK_SETTRIGGER5200] = true;
				
				break;
			}
			default:
				return 0;
		}
		
		this->updateBufferSize(this->specification.bufferSizes[this->settings.bufferSizeId]);
		this->setTriggerPosition(this->settings.trigger.position);
		return this->settings.samplerate.current;
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
		BulkSetFilter *commandSetFilter = (BulkSetFilter *) this->command[BULK_SETFILTER];
		commandSetFilter->setChannel(channel, !used);
		this->commandPending[BULK_SETFILTER] = true;
		
		unsigned char usedChannels = USED_CH1;
		if(!commandSetFilter->getChannel(1)) {
			if(commandSetFilter->getChannel(0))
				usedChannels = USED_CH2;
			else
				usedChannels = USED_CH1CH2;
		}
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGER5200: {
				// SetTrigger5200s bulk command for trigger source
				((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->setUsedChannels(usedChannels);
				this->commandPending[BULK_SETTRIGGER5200] = true;
				break;
			}
			default: {
				// SetTriggerAndSamplerate bulk command for trigger source
				((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->setUsedChannels(usedChannels);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
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
		for(gainId = 0; gainId < this->specification.gainSteps.count() - 1; gainId++)
			if(this->specification.gainSteps[gainId] >= gain)
				break;
		
		// SetGain bulk command for gain
		((BulkSetGain *) this->command[BULK_SETGAIN])->setGain(channel, this->specification.gainIndex[gainId]);
		this->commandPending[BULK_SETGAIN] = true;
		
		// SetRelays control command for gain relays
		ControlSetRelays *controlSetRelays = (ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS];
		controlSetRelays->setBelow1V(channel, gainId < 3);
		controlSetRelays->setBelow100mV(channel, gainId < 6);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		this->settings.voltage[channel].gain = gainId;
		
		this->setOffset(channel, this->settings.voltage[channel].offset);
		
		return this->specification.gainSteps[gainId];
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
		
		// Calculate the offset value
		// The range is given by the calibration data (convert from big endian)
		unsigned short int minimum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START]) + 1);
		unsigned short int maximum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END]) + 1);
		unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
		double offsetReal = (double) (offsetValue - minimum) / (maximum - minimum);
		
		// SetOffset control command for channel offset
		((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setChannel(channel, offsetValue);
		this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		
		this->settings.voltage[channel].offset = offset;
		this->settings.voltage[channel].offsetReal = offsetReal;
		
		this->setTriggerLevel(channel, this->settings.trigger.level[channel]);
		
		return offsetReal;
	}
	
	/// \brief Set the trigger mode.
	/// \return 0 on success, -1 on invalid mode.
	int Control::setTriggerMode(Dso::TriggerMode mode) {
		if(!this->device->isConnected())
			return -2;
		
		if(mode < Dso::TRIGGERMODE_AUTO || mode > Dso::TRIGGERMODE_SINGLE)
			return -1;
		
		this->settings.trigger.mode = mode;
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
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGER5200:
				// SetTrigger5200 bulk command for trigger source
				((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->setTriggerSource(sourceValue);
				this->commandPending[BULK_SETTRIGGER5200] = true;
				break;
			
			default:
				// SetTriggerAndSamplerate bulk command for trigger source
				((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerSource(sourceValue);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
		}
		
		// SetRelays control command for external trigger relay
		((ControlSetRelays *) this->control[CONTROLINDEX_SETRELAYS])->setTrigger(special);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		this->settings.trigger.special = special;
		this->settings.trigger.source = id;
		
		// Apply trigger level of the new source
		if(special) {
			// SetOffset control command for changed trigger level
			((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setTrigger(0x7f);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		else
			this->setTriggerLevel(id, this->settings.trigger.level[id]);
		
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
		
		// Calculate the trigger level value
		unsigned short int minimum, maximum;
		switch(this->device->getModel()) {
			case MODEL_DSO5200:
			case MODEL_DSO5200A:
				// The range is the same as used for the offsets for 10 bit models
				minimum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START]) + 1);
				maximum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END]) + 1);
				break;
			
			default:
				// It's from 0x00 to 0xfd for the 8 bit models
				minimum = 0x00;
				maximum = 0xfd;
				break;
		}
		
		// Never get out of the limits
		unsigned short int levelValue = qBound((long int) minimum, (long int) ((this->settings.voltage[channel].offsetReal + level / this->specification.gainSteps[this->settings.voltage[channel].gain]) * (maximum - minimum) + 0.5) + minimum, (long int) maximum);
		
		// Check if the set channel is the trigger source
		if(!this->settings.trigger.special && channel == this->settings.trigger.source) {
			// SetOffset control command for trigger level
			((ControlSetOffset *) this->control[CONTROLINDEX_SETOFFSET])->setTrigger(levelValue);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		
		/// \todo Get alternating trigger in here
		
		this->settings.trigger.level[channel] = level;
		return (double) ((levelValue - minimum) / (maximum - minimum) - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
	}
	
	/// \brief Set the trigger slope.
	/// \param slope The Slope that should cause a trigger.
	/// \return 0 on success, -1 on invalid slope.
	int Control::setTriggerSlope(Dso::Slope slope) {
		if(!this->device->isConnected())
			return -2;
		
		if(slope != Dso::SLOPE_NEGATIVE && slope != Dso::SLOPE_POSITIVE)
			return -1;
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGER5200: {
				// SetTrigger5200 bulk command for trigger slope
				((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->setTriggerSlope(slope);
				this->commandPending[BULK_SETTRIGGER5200] = true;
				break;
			}
			default: {
				// SetTriggerAndSamplerate bulk command for trigger slope
				BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = (BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE];
				
				commandSetTriggerAndSamplerate->setTriggerSlope(slope);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
			}
		}
		
		this->settings.trigger.slope = slope;
		return 0;
	}
	
	/// \brief Set the trigger position.
	/// \param position The new trigger position (in s).
	/// \return The trigger position that has been set.
	double Control::setTriggerPosition(double position) {
		if(!this->device->isConnected())
			return -2;
		
		// All trigger positions are measured in samples
		unsigned long int positionSamples = position * this->settings.samplerate.current;
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGER5200: {
				// Fast rate mode uses both channels
				if(((BulkSetTrigger5200 *) this->command[BULK_SETTRIGGER5200])->getFastRate())
					positionSamples /= HANTEK_CHANNELS;
				
				// Calculate the position values (Inverse, maximum is 0xffff)
				unsigned short int positionPre = 0xffff - this->specification.bufferSizes[this->settings.bufferSizeId] + positionSamples;
				unsigned short int positionPost = 0xffff - positionSamples;
				
				// SetBuffer5200 bulk command for trigger position
				BulkSetBuffer5200 *commandSetBuffer5200 = (BulkSetBuffer5200 *) this->command[BULK_SETBUFFER5200];
				commandSetBuffer5200->setTriggerPositionPre(positionPre);
				commandSetBuffer5200->setTriggerPositionPost(positionPost);
				this->commandPending[BULK_SETBUFFER5200] = true;
				
				break;
			}
			default: {
				// Fast rate mode uses both channels
				if(((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->getFastRate())
					positionSamples /= HANTEK_CHANNELS;
				
				// Calculate the position value (Start point depending on buffer size)
				unsigned long int position = 0x7ffff - this->specification.bufferSizes[this->settings.bufferSizeId] + positionSamples;
				
				// SetTriggerAndSamplerate bulk command for trigger position
				((BulkSetTriggerAndSamplerate *) this->command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(position);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			}
		}
		
		this->settings.trigger.position = position;
		return (double) positionSamples / this->settings.samplerate.current;
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
						if(commandCode > BULK_COUNT)
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
