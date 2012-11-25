////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  hantek/control.cpp
//
//  Copyright (C) 2008, 2009  Oleg Khudyakov
//  prcoder@potrebitel.ru
//  Copyright (C) 2010 - 2012  Oliver Haag
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


#include <cmath>
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
		this->specification.command.bulk.setRecordLength = (BulkCode) -1;
		this->specification.command.bulk.setChannels = (BulkCode) -1;
		this->specification.command.bulk.setGain = (BulkCode) -1;
		this->specification.command.bulk.setSamplerate = (BulkCode) -1;
		this->specification.command.bulk.setTrigger = (BulkCode) -1;
		this->specification.command.bulk.setPretrigger = (BulkCode) -1;
		this->specification.command.control.setOffset = (ControlCode) -1;
		this->specification.command.control.setRelays = (ControlCode) -1;
		this->specification.command.values.offsetLimits = (ControlValue) -1;
		this->specification.command.values.voltageLimits = (ControlValue) -1;
		
		this->specification.samplerate.single.base = 50e6;
		this->specification.samplerate.single.max = 50e6;
		this->specification.samplerate.single.recordLengths << 0;
		this->specification.samplerate.multi.base = 100e6;
		this->specification.samplerate.multi.max = 100e6;
		this->specification.samplerate.multi.recordLengths << 0;
		
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; ++channel) {
			for(unsigned int gainId = 0; gainId < 9; ++gainId) {
				this->specification.offsetLimit[channel][gainId][OFFSET_START] = 0x0000;
				this->specification.offsetLimit[channel][gainId][OFFSET_END] = 0xffff;
			}
		}
		
		// Set settings to default values
		this->settings.samplerate.limits = &(this->specification.samplerate.single);
		this->settings.samplerate.downsampler = 1;
		this->settings.samplerate.current = 1e8;
		this->settings.trigger.position = 0;
		this->settings.trigger.point = 0;
		this->settings.trigger.mode = Dso::TRIGGERMODE_NORMAL;
		this->settings.trigger.slope = Dso::SLOPE_POSITIVE;
		this->settings.trigger.special = false;
		this->settings.trigger.source = 0;
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; ++channel) {
			this->settings.trigger.level[channel] = 0.0;
			this->settings.voltage[channel].gain = 0;
			this->settings.voltage[channel].offset = 0.0;
			this->settings.voltage[channel].offsetReal = 0.0;
			this->settings.voltage[channel].used = false;
		}
		this->settings.recordLengthId = 0;
		this->settings.usedChannels = 0;
		
		// Special trigger sources
		this->specialTriggerSources << tr("EXT") << tr("EXT/10");
		
		// Instantiate bulk command later, some are not the same for all models
		for(int command = 0; command < BULK_COUNT; ++command) {
			this->command[command] = 0;
			this->commandPending[command] = false;
		}
		
		// Transmission-ready control commands
		this->control[CONTROLINDEX_SETOFFSET] = new ControlSetOffset();
		this->controlCode[CONTROLINDEX_SETOFFSET] = CONTROL_SETOFFSET;
		this->control[CONTROLINDEX_SETRELAYS] = new ControlSetRelays();
		this->controlCode[CONTROLINDEX_SETRELAYS] = CONTROL_SETRELAYS;
		
		for(int control = 0; control < CONTROLINDEX_COUNT; ++control)
			this->controlPending[control] = false;
		
		// USB device
		this->device = new Device(this);
		
		// Sample buffers
		for(unsigned int channel = 0; channel < HANTEK_CHANNELS; ++channel) {
			this->samples.append(0);
			this->samplesSize.append(0);
		}
		
		this->previousSampleCount = 0;
		
		connect(this->device, SIGNAL(disconnected()), this, SLOT(disconnectDevice()));
	}
	
	/// \brief Disconnects the device.
	Control::~Control() {
		this->device->disconnect();
		
		// Clean up commands
		for(int command = 0; command < BULK_COUNT; ++command) {
			if(this->command[command])
				delete this->command[command];
		}
	}
	
	/// \brief Gets the physical channel count for this oscilloscope.
	/// \return The number of physical channels.
	unsigned int Control::getChannelCount() {
		return HANTEK_CHANNELS;
	}
	
	/// \brief Get available record lengths for this oscilloscope.
	/// \return The number of physical channels, empty list for continuous.
	QList<unsigned long int> *Control::getAvailableRecordLengths() {
		return &this->settings.samplerate.limits->recordLengths;
	}
	
	/// \brief Get minimum samplerate for this oscilloscope.
	/// \return The minimum samplerate for the current configuration in S/s.
	double Control::getMinSamplerate() {
		return (double) this->specification.samplerate.single.base / this->specification.samplerate.single.maxDownsampler;
	}

	/// \brief Get maximum samplerate for this oscilloscope.
	/// \return The maximum samplerate for the current configuration in S/s.
	double Control::getMaxSamplerate() {
		ControlSamplerateLimits *limits = (this->settings.usedChannels <= 1) ? &this->specification.samplerate.multi : &this->specification.samplerate.single;
		return limits->max;
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
			for(int command = 0; command < BULK_COUNT; ++command) {
				if(!this->commandPending[command])
					continue;
				
#ifdef DEBUG
				qDebug("Sending bulk command:%s", Helper::hexDump(this->command[command]->data(), this->command[command]->getSize()).toLocal8Bit().data());
#endif
				
				errorCode = this->device->bulkCommand(this->command[command]);
				if(errorCode < 0) {
					qWarning("Sending bulk command %02x failed: %s", command, Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
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
			for(int control = 0; control < CONTROLINDEX_COUNT; ++control) {
				if(!this->controlPending[control])
					continue;
				
#ifdef DEBUG
				qDebug("Sending control command %02x:%s", this->controlCode[control], Helper::hexDump(this->control[control]->data(), this->control[control]->getSize()).toLocal8Bit().data());
#endif
				
				errorCode = this->device->controlWrite(this->controlCode[control], this->control[control]->data(), this->control[control]->getSize());
				if(errorCode < 0) {
					qWarning("Sending control command %2x failed: %s", this->controlCode[control], Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
					
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
			int cycleTime = qMax((unsigned long int) (this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId] / this->settings.samplerate.current * 250), 10lu);
			this->msleep(cycleTime);
			
			if(!this->sampling) {
				samplingStarted = false;
				continue;
			}
			
#ifdef DEBUG
			int lastCaptureState = captureState;
#endif
			captureState = this->getCaptureState();
			if(captureState < 0)
				qWarning("Getting capture state failed: %s", Helper::libUsbErrorString(captureState).toLocal8Bit().data());
#ifdef DEBUG
			else if(captureState != lastCaptureState)
				qDebug("Capture state changed to %d", captureState);
#endif
			switch(captureState) {
				case CAPTURE_READY:
				case CAPTURE_READY2250:
				case CAPTURE_READY5200:
					// Get data and process it, if we're still sampling
					errorCode = this->getSamples(samplingStarted);
					if(errorCode < 0)
						qWarning("Getting sample data failed: %s", Helper::libUsbErrorString(errorCode).toLocal8Bit().data());
#ifdef DEBUG
					else
						qDebug("Received %d B of sampling data", errorCode);
#endif
					
					// Check if we're in single trigger mode
					if(this->settings.trigger.mode == Dso::TRIGGERMODE_SINGLE && samplingStarted)
						this->stopSampling();
					
					// Sampling completed, restart it when necessary
					samplingStarted = false;
					
					// Start next capture if necessary by leaving out the break statement
					if(!this->sampling)
						break;
				
				case CAPTURE_WAITING:
					// Sampling hasn't started, update the expected sample count
					if(this->settings.samplerate.limits == &this->specification.samplerate.multi)
						this->previousSampleCount = this->specification.samplerate.multi.recordLengths[this->settings.recordLengthId];
					else
						this->previousSampleCount = this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] * HANTEK_CHANNELS;
					
					if(samplingStarted && lastTriggerMode == this->settings.trigger.mode) {
						++cycleCounter;
						
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
						
						if(cycleCounter < 20 || cycleCounter < 4000 / cycleTime)
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
					break;
			}
		}
		
		this->device->disconnect();
		emit statusMessage(tr("The device has been disconnected"), 0);
	}
	
	/// \brief Calculates the trigger point from the CommandGetCaptureState data.
	/// \param value The data value that contains the trigger point.
	/// \return The calculated trigger point for the given data.
	unsigned long int Control::calculateTriggerPoint(unsigned long int value) {
		unsigned long int result = value;

		// Each set bit inverts all bits with a lower value
		for(unsigned long int bitValue = 1; bitValue; bitValue <<= 1)
			if(result & bitValue)
				result ^= bitValue - 1;

		return result;
	}
	
	/// \brief Gets the current state.
	/// \return The current CaptureState of the oscilloscope, libusb error code on error.
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
	/// \return sample count on success, libusb error code on error.
	int Control::getSamples(bool process) {
		int errorCode;
		
		// Request data
		errorCode = this->device->bulkCommand(this->command[BULK_GETDATA], 1);
		if(errorCode < 0)
			return errorCode;
		
		// Save raw data to temporary buffer
		bool fastRate = this->settings.samplerate.limits == &this->specification.samplerate.multi;
		
		unsigned long int totalSampleCount = fastRate ? this->specification.samplerate.multi.recordLengths[this->settings.recordLengthId] : this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] * HANTEK_CHANNELS;
		
		// To make sure no samples will remain in the scope buffer, also check the sample count before the last sampling started
		if(totalSampleCount < this->previousSampleCount) {
			unsigned long int currentSampleCount = totalSampleCount;
			totalSampleCount = this->previousSampleCount;
			this->previousSampleCount = currentSampleCount; // Using sampleCount as temporary buffer since it was set to totalSampleCount
		}
		else {
			this->previousSampleCount = totalSampleCount;
		}
		
		unsigned long int sampleCount = totalSampleCount;
		if(!fastRate)
			sampleCount /= HANTEK_CHANNELS;
		unsigned long int dataLength = totalSampleCount;
		if(this->specification.sampleSize > 8)
			dataLength *= 2;
		
		unsigned char data[dataLength];
		errorCode = this->device->bulkReadMulti(data, dataLength);
		if(errorCode < 0)
			return errorCode;
		
		// Process the data only if we want it
		if(process) {
			// How much data did we really receive?
			dataLength = errorCode;
			if(this->specification.sampleSize > 8)
				totalSampleCount = dataLength / 2;
			else
				totalSampleCount = dataLength;
			
			this->samplesMutex.lock();
			
			// Convert channel data
			if(fastRate) {
				// Fast rate mode, one channel is using all buffers
				sampleCount = totalSampleCount;
				int channel = 0;
				for(; channel < HANTEK_CHANNELS; ++channel) {
					if(this->settings.voltage[channel].used)
						break;
				}
				
				// Clear unused channels
				for(int channelCounter = 0; channelCounter < HANTEK_CHANNELS; ++channelCounter)
					if(channelCounter != channel && this->samples[channelCounter]) {
						
						delete this->samples[channelCounter];
						this->samples[channelCounter] = 0;
					}
				
				if(channel < HANTEK_CHANNELS) {
					// Reallocate memory for samples if the sample count has changed
					if(!this->samples[channel] || this->samplesSize[channel] != sampleCount) {
						if(this->samples[channel])
							delete this->samples[channel];
						this->samples[channel] = new double[sampleCount];
						this->samplesSize[channel] = sampleCount;
					}
					
					// Convert data from the oscilloscope and write it into the sample buffer
					unsigned long int bufferPosition = this->settings.trigger.point * 2;
					if(this->specification.sampleSize > 8) {
						// Additional most significant bits after the normal data
						unsigned int extraBitsPosition; // Track the position of the extra bits in the additional byte
						unsigned int extraBitsSize = this->specification.sampleSize - 8; // Number of extra bits
						unsigned short int extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction
						
						for(unsigned long int realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
							if(bufferPosition >= sampleCount)
								bufferPosition %= sampleCount;
							
							extraBitsPosition = bufferPosition % HANTEK_CHANNELS;
							
							this->samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition] + (((unsigned short int) data[sampleCount + bufferPosition - extraBitsPosition] << (8 - (HANTEK_CHANNELS - 1 - extraBitsPosition) * extraBitsSize)) & extraBitsMask)) / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
						}
					}
					else {
						for(unsigned long int realPosition = 0; realPosition < sampleCount; ++realPosition, ++bufferPosition) {
							if(bufferPosition >= sampleCount)
								bufferPosition %= sampleCount;
							
							this->samples[channel][realPosition] = ((double) data[bufferPosition] / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
						}
					}
				}
			}
			else {
				// Normal mode, channels are using their separate buffers
				sampleCount = totalSampleCount / HANTEK_CHANNELS;
				for(int channel = 0; channel < HANTEK_CHANNELS; ++channel) {
					if(this->settings.voltage[channel].used) {
						// Reallocate memory for samples if the sample count has changed
						if(!this->samples[channel] || this->samplesSize[channel] != sampleCount) {
							if(this->samples[channel])
								delete this->samples[channel];
							this->samples[channel] = new double[sampleCount];
							this->samplesSize[channel] = sampleCount;
						}
						
						// Convert data from the oscilloscope and write it into the sample buffer
						unsigned long int bufferPosition = this->settings.trigger.point * 2;
						if(this->specification.sampleSize > 8) {
							// Additional most significant bits after the normal data
							unsigned int extraBitsSize = this->specification.sampleSize - 8; // Number of extra bits
							unsigned short int extraBitsMask = (0x00ff << extraBitsSize) & 0xff00; // Mask for extra bits extraction
							unsigned int extraBitsIndex = 8 - channel * 2; // Bit position offset for extra bits extraction
							
							for(unsigned long int realPosition = 0; realPosition < sampleCount; ++realPosition, bufferPosition += HANTEK_CHANNELS) {
								if(bufferPosition >= totalSampleCount)
									bufferPosition %= totalSampleCount;
								
								this->samples[channel][realPosition] = ((double) ((unsigned short int) data[bufferPosition + HANTEK_CHANNELS - 1 - channel] + (((unsigned short int) data[totalSampleCount + bufferPosition] << extraBitsIndex) & extraBitsMask)) / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
							}
						}
						else {
							bufferPosition += HANTEK_CHANNELS - 1 - channel;
							for(unsigned long int realPosition = 0; realPosition < sampleCount; ++realPosition, bufferPosition += HANTEK_CHANNELS) {
								if(bufferPosition >= totalSampleCount)
									bufferPosition %= totalSampleCount;
								
								this->samples[channel][realPosition] = ((double) data[bufferPosition] / this->specification.voltageLimit[channel][this->settings.voltage[channel].gain] - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
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
		
		return errorCode;
	}
	
	/// \brief Calculated the nearest samplerate supported by the oscilloscope.
	/// \param samplerate The target samplerate, that should be met as good as possible.
	/// \param fastRate true, if the fast rate mode is enabled.
	/// \param maximum The target samplerate is the maximum allowed when true, the minimum otherwise.
	/// \param downsampler Pointer to where the selected downsampling factor should be written.
	/// \return The nearest samplerate supported, 0.0 on error.
	double Control::getBestSamplerate(double samplerate, bool fastRate, bool maximum, unsigned long int *downsampler) {
		// Abort if the input value is invalid
		if(samplerate <= 0.0)
			return 0.0;
		
		double bestSamplerate = 0.0;
		
		// Get samplerate specifications for this mode and model
		ControlSamplerateLimits *limits;
		if(fastRate)
			limits = &(this->specification.samplerate.multi);
		else
			limits = &(this->specification.samplerate.single);
		
		// Get downsampling factor that would provide the requested rate
		double bestDownsampler = (double) limits->base / this->specification.bufferDividers[this->settings.recordLengthId] / samplerate;
		// Base samplerate sufficient, or is the maximum better?
		if(bestDownsampler < 1.0 && (samplerate <= limits->max / this->specification.bufferDividers[this->settings.recordLengthId] || !maximum)) {
			bestDownsampler = 0.0;
			bestSamplerate = limits->max / this->specification.bufferDividers[this->settings.recordLengthId];
		}
		else {
			switch(this->specification.command.bulk.setSamplerate) {
				case BULK_SETTRIGGERANDSAMPLERATE:
					// DSO-2090 supports the downsampling factors 1, 2, 4 and 5 using valueFast or all even values above using valueSlow
					if((maximum && bestDownsampler <= 5.0) || (!maximum && bestDownsampler < 6.0)) {
						// valueFast is used
						if(maximum) {
							// The samplerate shall not be higher, so we round up
							bestDownsampler = ceil(bestDownsampler);
							if(bestDownsampler > 2.0) // 3 and 4 not possible with the DSO-2090
								bestDownsampler = 5.0;
						}
						else {
							// The samplerate shall not be lower, so we round down
							bestDownsampler = floor(bestDownsampler);
							if(bestDownsampler > 2.0 && bestDownsampler < 5.0) // 3 and 4 not possible with the DSO-2090
								bestDownsampler = 2.0;
						}
					}
					else {
						// valueSlow is used
						if(maximum) {
							bestDownsampler = ceil(bestDownsampler / 2.0) * 2.0; // Round up to next even value
						}
						else {
							bestDownsampler = floor(bestDownsampler / 2.0) * 2.0; // Round down to next even value
						}
						if(bestDownsampler > 2.0 * 0x10001) // Check for overflow
							bestDownsampler = 2.0 * 0x10001;
					}
					break;
				
				case BULK_CSETTRIGGERORSAMPLERATE:
					// DSO-5200 may not supports all downsampling factors, requires testing
					if(maximum) {
						bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
					}
					else {
						bestDownsampler = floor(bestDownsampler); // Round down to next integer value
					}
					break;
				
				case BULK_ESETTRIGGERORSAMPLERATE:
					// DSO-2250 doesn't have a fast value, so it supports all downsampling factors
					if(maximum) {
						bestDownsampler = ceil(bestDownsampler); // Round up to next integer value
					}
					else {
						bestDownsampler = floor(bestDownsampler); // Round down to next integer value
					}
					break;
				
				default:
					return 0.0;
			}
			
			// Limit maximum downsampler value to avoid overflows in the sent commands
			if(bestDownsampler > limits->maxDownsampler)
				bestDownsampler = limits->maxDownsampler;
		
			bestSamplerate = limits->base / bestDownsampler / this->specification.bufferDividers[this->settings.recordLengthId];
		}
		
		if(downsampler)
			*downsampler = (unsigned long int) bestDownsampler;
		return bestSamplerate;
	}
	
	/// \brief Sets the size of the sample buffer without updating dependencies.
	/// \param index The record length index that should be set.
	/// \return The record length that has been set, 0 on error.
	unsigned long int Control::updateRecordLength(unsigned long int index) {
		if(index >= (unsigned long int) this->settings.samplerate.limits->recordLengths.size())
			return 0;
		
		switch(this->specification.command.bulk.setRecordLength) {
			case BULK_SETTRIGGERANDSAMPLERATE:
				// SetTriggerAndSamplerate bulk command for record length
				static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE])->setRecordLength(index);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			
			case BULK_DSETBUFFER:
				if(this->specification.command.bulk.setPretrigger == BULK_FSETBUFFER) {
					// Pointers to needed commands
					BulkSetRecordLength2250 *commandSetRecordLength2250 = static_cast<BulkSetRecordLength2250 *>(this->command[BULK_DSETBUFFER]);
					
					commandSetRecordLength2250->setRecordLength(index);
				}
				else {
					// SetBuffer5200 bulk command for record length
					BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(this->command[BULK_DSETBUFFER]);
					
					commandSetBuffer5200->setUsedPre(DTRIGGERPOSITION_ON);
					commandSetBuffer5200->setUsedPost(DTRIGGERPOSITION_ON);
					commandSetBuffer5200->setRecordLength(index);
				}
				
				this->commandPending[BULK_DSETBUFFER] = true;
				
				break;
			
			default:
				return 0;
		}
		
		this->settings.recordLengthId = index;
		
		return this->settings.samplerate.limits->recordLengths[index];
	}
	
	/// \brief Sets the samplerate based on the parameters calculated by Control::getBestSamplerate.
	/// \param downsampler The downsampling factor.
	/// \param fastRate true, if one channel uses all buffers.
	/// \return The downsampling factor that has been set.
	unsigned long int Control::updateSamplerate(unsigned long int downsampler, bool fastRate) {
		// Set the calculated samplerate
		switch(this->specification.command.bulk.setSamplerate) {
			case BULK_SETTRIGGERANDSAMPLERATE: {
				short int downsamplerValue = 0;
				unsigned char samplerateId = 0;
				bool downsampling = false;
				
				if(downsampler <= 5) {
					// All dividers up to 5 are done using the special samplerate IDs
					if(downsampler == 0)
						samplerateId = 1;
					else if(downsampler <= 2)
						samplerateId = downsampler;
					else { // Downsampling factors 3 and 4 are not supported
						samplerateId = 3;
						downsampler = 5;
						downsamplerValue = 0xffff;
					}
				}
				else {
					// For any dividers above the downsampling factor can be set directly
					downsampler &= ~0x0001; // Only even values possible
					downsamplerValue = (short int) (0x10001 - (downsampler >> 1));
					
					downsampling = true;
				}
				
				// Pointers to needed commands
				BulkSetTriggerAndSamplerate *commandSetTriggerAndSamplerate = static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE]);
				
				// Store if samplerate ID or downsampling factor is used
				commandSetTriggerAndSamplerate->setDownsamplingMode(downsampling);
				// Store samplerate ID
				commandSetTriggerAndSamplerate->setSamplerateId(samplerateId);
				// Store downsampling factor
				commandSetTriggerAndSamplerate->setDownsampler(downsamplerValue);
				// Set fast rate when used
				commandSetTriggerAndSamplerate->setFastRate(false /*fastRate*/);
				
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			}
			case BULK_CSETTRIGGERORSAMPLERATE: {
				// Split the resulting divider into the values understood by the device
				// The fast value is kept at 4 (or 3) for slow sample rates
				long int valueSlow = qMax(((long int) downsampler - 3) / 2, (long int) 0);
				unsigned char valueFast = downsampler - valueSlow * 2;
				
				// Pointers to needed commands
				BulkSetSamplerate5200 *commandSetSamplerate5200 = static_cast<BulkSetSamplerate5200 *>(this->command[BULK_CSETTRIGGERORSAMPLERATE]);
				BulkSetTrigger5200 *commandSetTrigger5200 = static_cast<BulkSetTrigger5200 *>(this->command[BULK_ESETTRIGGERORSAMPLERATE]);
				
				// Store samplerate fast value
				commandSetSamplerate5200->setSamplerateFast(4 - valueFast);
				// Store samplerate slow value (two's complement)
				commandSetSamplerate5200->setSamplerateSlow(valueSlow == 0 ? 0 : 0xffff - valueSlow);
				// Set fast rate when used
				commandSetTrigger5200->setFastRate(fastRate);
				
				this->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				
				break;
			}
			case BULK_ESETTRIGGERORSAMPLERATE: {
				// Pointers to needed commands
				BulkSetSamplerate2250 *commandSetSamplerate2250 = static_cast<BulkSetSamplerate2250 *>(this->command[BULK_ESETTRIGGERORSAMPLERATE]);
				
				bool downsampling = downsampler >= 1;
				// Store downsampler state value
				commandSetSamplerate2250->setDownsampling(downsampling);
				// Store samplerate value
				commandSetSamplerate2250->setSamplerate(downsampler > 1 ? 0x10001 - downsampler : 0);
				// Set fast rate when used
				commandSetSamplerate2250->setFastRate(fastRate);
				
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				
				break;
			}
			default:
				return ULONG_MAX;
		}
		
		// Update settings
		bool fastRateChanged = fastRate != (this->settings.samplerate.limits == &this->specification.samplerate.multi);
		if(fastRateChanged) {
			if(fastRate)
				this->settings.samplerate.limits = &this->specification.samplerate.multi;
			else
				this->settings.samplerate.limits = &this->specification.samplerate.single;
		}
		
		this->settings.samplerate.downsampler = downsampler;
		if(downsampler)
			this->settings.samplerate.current = this->settings.samplerate.limits->base / downsampler;
		else
			this->settings.samplerate.current = this->settings.samplerate.limits->max;
		
		// Update dependencies
		this->setPretriggerPosition(this->settings.trigger.position);
		
		// Emit signals for changed settings
		if(fastRateChanged) {
			emit availableRecordLengthsChanged(this->settings.samplerate.limits->recordLengths);
			emit recordLengthChanged(this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId]);
		}
		emit recordTimeChanged((double) this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId] / this->settings.samplerate.current);
		emit samplerateChanged(this->settings.samplerate.current);
		
		return downsampler;
	}
	
	/// \brief Try to connect to the oscilloscope.
	void Control::restoreTargets() {
		if(this->settings.samplerate.target.samplerateSet)
			this->setSamplerate();
		else
			this->setRecordTime();
	}
	
	/// \brief Try to connect to the oscilloscope.
	void Control::connectDevice() {
		int errorCode;
		
		emit statusMessage(this->device->search(), 0);
		if(!this->device->isConnected())
			return;
		
		// Clean up commands and their pending state
		for(int command = 0; command < BULK_COUNT; ++command) {
			if(this->command[command])
				delete this->command[command];
			this->commandPending[command] = false;
		}
		// Instantiate the commands needed for all models
		this->command[BULK_FORCETRIGGER] = new BulkForceTrigger();
		this->command[BULK_STARTSAMPLING] = new BulkCaptureStart();
		this->command[BULK_ENABLETRIGGER] = new BulkTriggerEnabled();
		this->command[BULK_GETDATA] = new BulkGetData();
		this->command[BULK_GETCAPTURESTATE] = new BulkGetCaptureState();
		this->command[BULK_SETGAIN] = new BulkSetGain();
		// Initialize the command versions to the ones used on the DSO-2090
		this->specification.command.bulk.setRecordLength = (BulkCode) -1;
		this->specification.command.bulk.setChannels = (BulkCode) -1;
		this->specification.command.bulk.setGain = BULK_SETGAIN;
		this->specification.command.bulk.setSamplerate = (BulkCode) -1;
		this->specification.command.bulk.setTrigger = (BulkCode) -1;
		this->specification.command.bulk.setPretrigger = (BulkCode) -1;
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
				// Instantiate additional commands for the DSO-2090
				this->command[BULK_SETTRIGGERANDSAMPLERATE] = new BulkSetTriggerAndSamplerate();
				this->specification.command.bulk.setRecordLength = BULK_SETTRIGGERANDSAMPLERATE;
				this->specification.command.bulk.setChannels = BULK_SETTRIGGERANDSAMPLERATE;
				this->specification.command.bulk.setSamplerate = BULK_SETTRIGGERANDSAMPLERATE;
				this->specification.command.bulk.setTrigger = BULK_SETTRIGGERANDSAMPLERATE;
				this->specification.command.bulk.setPretrigger = BULK_SETTRIGGERANDSAMPLERATE;
				// Initialize those as pending
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
			
			case MODEL_DSO2250:
				// Instantiate additional commands for the DSO-2250
				this->command[BULK_BSETCHANNELS] = new BulkSetChannels2250();
				this->command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetTrigger2250();
				this->command[BULK_DSETBUFFER] = new BulkSetRecordLength2250();
				this->command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetSamplerate2250();
				this->command[BULK_FSETBUFFER] = new BulkSetBuffer2250();
				this->specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
				this->specification.command.bulk.setChannels = BULK_BSETCHANNELS;
				this->specification.command.bulk.setSamplerate = BULK_ESETTRIGGERORSAMPLERATE;
				this->specification.command.bulk.setTrigger = BULK_CSETTRIGGERORSAMPLERATE;
				this->specification.command.bulk.setPretrigger = BULK_FSETBUFFER;
				
				this->commandPending[BULK_BSETCHANNELS] = true;
				this->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
				this->commandPending[BULK_DSETBUFFER] = true;
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				this->commandPending[BULK_FSETBUFFER] = true;
				
				break;
			
			case MODEL_DSO5200A:
				unsupported = true;
			
			case MODEL_DSO5200:
				// Instantiate additional commands for the DSO-5200
				this->command[BULK_CSETTRIGGERORSAMPLERATE] = new BulkSetSamplerate5200();
				this->command[BULK_DSETBUFFER] = new BulkSetBuffer5200();
				this->command[BULK_ESETTRIGGERORSAMPLERATE] = new BulkSetTrigger5200();
				this->specification.command.bulk.setRecordLength = BULK_DSETBUFFER;
				this->specification.command.bulk.setChannels = BULK_ESETTRIGGERORSAMPLERATE;
				this->specification.command.bulk.setSamplerate = BULK_CSETTRIGGERORSAMPLERATE;
				this->specification.command.bulk.setTrigger = BULK_ESETTRIGGERORSAMPLERATE;
				this->specification.command.bulk.setPretrigger = BULK_ESETTRIGGERORSAMPLERATE;
				//this->specification.command.values.voltageLimits = VALUE_ETSCORRECTION;
				
				this->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
				this->commandPending[BULK_DSETBUFFER] = true;
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				
				break;
			
			default:
				this->device->disconnect();
				emit statusMessage(tr("Unknown model"), 0);
				return;
		}
		
		if(unsupported)
			qWarning("Warning: This Hantek DSO model isn't supported officially, so it may not be working as expected. Reports about your experiences are very welcome though (Please open a feature request in the tracker at https://sf.net/projects/openhantek/ or email me directly to oliver.haag@gmail.com). If it's working perfectly I can remove this warning, if not it should be possible to get it working with your help soon.");
		
		for(int control = 0; control < CONTROLINDEX_COUNT; ++control)
			this->controlPending[control] = true;
		
		// Maximum possible samplerate for a single channel and dividers for record lengths
		this->specification.bufferDividers.clear();
		this->specification.samplerate.single.recordLengths.clear();
		this->specification.samplerate.multi.recordLengths.clear();
		this->specification.gainSteps.clear();
		for(int channel = 0; channel < HANTEK_CHANNELS; ++channel)
			this->specification.voltageLimit[channel].clear();
		
		switch(this->device->getModel()) {
			case MODEL_DSO5200:
			case MODEL_DSO5200A:
				this->specification.samplerate.single.base = 100e6;
				this->specification.samplerate.single.max = 125e6;
				this->specification.samplerate.single.maxDownsampler = 131072;
				this->specification.samplerate.single.recordLengths << ULONG_MAX << 10240 << 14336;
				this->specification.samplerate.multi.base = 200e6;
				this->specification.samplerate.multi.max = 250e6;
				this->specification.samplerate.multi.maxDownsampler = 131072;
				this->specification.samplerate.multi.recordLengths << ULONG_MAX << 20480 << 28672;
				this->specification.bufferDividers << 1000 << 1 << 1;
				this->specification.gainSteps
					<< 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0 << 80.0;
				/// \todo Use calibration data to get the DSO-5200(A) sample ranges
				for(int channel = 0; channel < HANTEK_CHANNELS; ++channel)
					this->specification.voltageLimit[channel]
					<<  368 <<  454 <<  908 <<  368 <<  454 <<  908 <<  368 <<  454 <<  908;
				this->specification.gainIndex
					<<    1 <<    0 <<    0 <<    1 <<    0 <<    0 <<    1 <<    0 <<    0;
				this->specification.sampleSize = 10;
				break;
			
			case MODEL_DSO2250:
				this->specification.samplerate.single.base = 100e6;
				this->specification.samplerate.single.max = 100e6;
				this->specification.samplerate.single.maxDownsampler = 65536;
				this->specification.samplerate.single.recordLengths << ULONG_MAX << 10240 << 524288;
				this->specification.samplerate.multi.base = 200e6;
				this->specification.samplerate.multi.max = 250e6;
				this->specification.samplerate.multi.maxDownsampler = 65536;
				this->specification.samplerate.multi.recordLengths << ULONG_MAX << 20480 << 1048576;
				this->specification.bufferDividers << 1000 << 1 << 1;
				this->specification.gainSteps
					<< 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
				for(int channel = 0; channel < HANTEK_CHANNELS; ++channel)
					this->specification.voltageLimit[channel]
					<<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
				this->specification.gainIndex
					<<    0 <<    2 <<    3 <<    0 <<    2 <<    3 <<    0 <<    2 <<    3;
				this->specification.sampleSize = 8;
				break;
			
			case MODEL_DSO2150:
				this->specification.samplerate.single.base = 50e6;
				this->specification.samplerate.single.max = 75e6;
				this->specification.samplerate.single.maxDownsampler = 131072;
				this->specification.samplerate.single.recordLengths << ULONG_MAX << 10240 << 32768;
				this->specification.samplerate.multi.base = 100e6;
				this->specification.samplerate.multi.max = 150e6;
				this->specification.samplerate.multi.maxDownsampler = 131072;
				this->specification.samplerate.multi.recordLengths << ULONG_MAX << 20480 << 65536;
				this->specification.bufferDividers << 1000 << 1 << 1;
				this->specification.gainSteps
					<< 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
				for(int channel = 0; channel < HANTEK_CHANNELS; ++channel)
					this->specification.voltageLimit[channel]
					<<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
				this->specification.gainIndex
					<<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;
				this->specification.sampleSize = 8;
				break;
			
			default:
				this->specification.samplerate.single.base = 50e6;
				this->specification.samplerate.single.max = 50e6;
				this->specification.samplerate.single.maxDownsampler = 131072;
				this->specification.samplerate.single.recordLengths << ULONG_MAX << 10240 << 32768;
				this->specification.samplerate.multi.base = 100e6;
				this->specification.samplerate.multi.max = 100e6;
				this->specification.samplerate.multi.maxDownsampler = 131072;
				this->specification.samplerate.multi.recordLengths << ULONG_MAX << 20480 << 65536;
				this->specification.bufferDividers << 1000 << 1 << 1;
				this->specification.gainSteps
					<< 0.08 << 0.16 << 0.40 << 0.80 << 1.60 << 4.00 <<  8.0 << 16.0 << 40.0;
				for(int channel = 0; channel < HANTEK_CHANNELS; ++channel)
					this->specification.voltageLimit[channel]
					<<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255 <<  255;
				this->specification.gainIndex
					<<    0 <<    1 <<    2 <<    0 <<    1 <<    2 <<    0 <<    1 <<    2;
				this->specification.sampleSize = 8;
				break;
		}
		this->settings.recordLengthId = 1;
		this->settings.samplerate.limits = &(this->specification.samplerate.single);
		this->settings.samplerate.downsampler = 1;
		this->previousSampleCount = 0;
		
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
	/// \param index The record length index that should be set.
	/// \return The record length that has been set, 0 on error.
	unsigned long int Control::setRecordLength(unsigned long int index) {
	if(!this->device->isConnected())
			return 0;
		
		if(!this->updateRecordLength(index))
			return 0;
		
		this->restoreTargets();
		this->setPretriggerPosition(this->settings.trigger.position);
		
		emit recordLengthChanged(this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId]);
		return this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId];
	}
	
	/// \brief Sets the samplerate of the oscilloscope.
	/// \param samplerate The samplerate that should be met (S/s), 0.0 to restore current samplerate.
	/// \return The samplerate that has been set, 0.0 on error.
	double Control::setSamplerate(double samplerate) {
		if(samplerate == 0.0) {
			samplerate = this->settings.samplerate.target.samplerate;
		}
		else {
			this->settings.samplerate.target.samplerate = samplerate;
			this->settings.samplerate.target.samplerateSet = true;
		}
		
		// When possible, enable fast rate if it is required to reach the requested samplerate
		bool fastRate = (this->settings.usedChannels <= 1) && (samplerate > this->specification.samplerate.single.max);
		
		// What is the nearest, at least as high samplerate the scope can provide?
		unsigned long int downsampler = 0;
		double bestSamplerate = getBestSamplerate(samplerate, fastRate, false, &(downsampler));
		
		// Set the calculated samplerate
		if(this->updateSamplerate(downsampler, fastRate) == ULONG_MAX)
			return 0.0;
		else {
			return bestSamplerate;
		}
	}
	
	/// \brief Sets the time duration of one aquisition by adapting the samplerate.
	/// \param duration The record time duration that should be met (s), 0.0 to restore current record time.
	/// \return The record time duration that has been set, 0.0 on error.
	double Control::setRecordTime(double duration) {
		if(duration == 0.0) {
			duration = this->settings.samplerate.target.duration;
		}
		else {
			this->settings.samplerate.target.duration = duration;
			this->settings.samplerate.target.samplerateSet = false;
		}
		
		// Calculate the maximum samplerate that would still provide the requested duration
		double maxSamplerate = (double) this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] / duration;
		
		// When possible, enable fast rate if the record time can't be set that low to improve resolution
		bool fastRate = (this->settings.usedChannels <= 1) && (maxSamplerate >= this->specification.samplerate.multi.base);
		
		// What is the nearest, at most as high samplerate the scope can provide?
		unsigned long int downsampler = 0;
		double bestSamplerate = getBestSamplerate(maxSamplerate, fastRate, true, &(downsampler));
		
		// Set the calculated samplerate
		if(this->updateSamplerate(downsampler, fastRate) == ULONG_MAX)
			return 0.0;
		else {
			return (double) this->settings.samplerate.limits->recordLengths[this->settings.recordLengthId] / bestSamplerate;
		}
	}
	
	/// \brief Enables/disables filtering of the given channel.
	/// \param channel The channel that should be set.
	/// \param used true if the channel should be sampled.
	/// \return See ::Dso::ErrorCode.
	int Control::setChannelUsed(unsigned int channel, bool used) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(channel >= HANTEK_CHANNELS)
			return Dso::ERROR_PARAMETER;
		
		// Update settings
		this->settings.voltage[channel].used = used;
		unsigned int channelCount = 0;
		for(int channelCounter = 0; channelCounter < HANTEK_CHANNELS; ++channelCounter) {
			if(this->settings.voltage[channelCounter].used)
				++channelCount;
		}
		
		// Calculate the UsedChannels field for the command
		unsigned char usedChannels = USED_CH1;
		
		if(this->settings.voltage[1].used) {
			if(this->settings.voltage[0].used) {
				usedChannels = USED_CH1CH2;
			}
			else {
				// DSO-2250 uses a different value for channel 2
				if(this->specification.command.bulk.setTrigger == BULK_BSETCHANNELS)
					usedChannels = BUSED_CH2;
				else
					usedChannels = USED_CH2;
			}
		}
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGERANDSAMPLERATE: {
				// SetTriggerAndSamplerate bulk command for trigger source
				static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE])->setUsedChannels(usedChannels);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
			}
			case BULK_BSETCHANNELS: {
				// SetChannels2250 bulk command for active channels
				static_cast<BulkSetChannels2250 *>(this->command[BULK_BSETCHANNELS])->setUsedChannels(usedChannels);
				this->commandPending[BULK_BSETCHANNELS] = true;
				
				break;
			}
			case BULK_ESETTRIGGERORSAMPLERATE: {
				// SetTrigger5200s bulk command for trigger source
				static_cast<BulkSetTrigger5200 *>(this->command[BULK_ESETTRIGGERORSAMPLERATE])->setUsedChannels(usedChannels);
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				break;
			}
			default:
				break;
		}
		
		// Check if fast rate mode availability changed
		bool fastRateChanged = (this->settings.usedChannels <= 1) != (channelCount <= 1);
		this->settings.usedChannels = channelCount;
		
		if(fastRateChanged) {
			// Works only if the minimum samplerate for normal mode is lower than for fast rate mode, which is the case for all models
			ControlSamplerateLimits *limits = (channelCount <= 1) ? &this->specification.samplerate.multi : &this->specification.samplerate.single;
			emit samplerateLimitsChanged((double) this->specification.samplerate.single.base / this->specification.samplerate.single.maxDownsampler, limits->max);
			
			// Samplerate differs for fast rate mode, recalculate it
			this->restoreTargets();
		}
		
		return Dso::ERROR_NONE;
	}
	
	/// \brief Set the coupling for the given channel.
	/// \param channel The channel that should be set.
	/// \param coupling The new coupling for the channel.
	/// \return See ::Dso::ErrorCode.
	int Control::setCoupling(unsigned int channel, Dso::Coupling coupling) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(channel >= HANTEK_CHANNELS)
			return Dso::ERROR_PARAMETER;
		
		// SetRelays control command for coupling relays
		static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS])->setCoupling(channel, coupling != Dso::COUPLING_AC);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		return Dso::ERROR_NONE;
	}
	
	/// \brief Sets the gain for the given channel.
	/// \param channel The channel that should be set.
	/// \param gain The gain that should be met (V/div).
	/// \return The gain that has been set, ::Dso::ErrorCode on error.
	double Control::setGain(unsigned int channel, double gain) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(channel >= HANTEK_CHANNELS)
			return Dso::ERROR_PARAMETER;
		
		// Find lowest gain voltage thats at least as high as the requested
		int gainId;
		for(gainId = 0; gainId < this->specification.gainSteps.count() - 1; ++gainId)
			if(this->specification.gainSteps[gainId] >= gain)
				break;
		
		// SetGain bulk command for gain
		static_cast<BulkSetGain *>(this->command[BULK_SETGAIN])->setGain(channel, this->specification.gainIndex[gainId]);
		this->commandPending[BULK_SETGAIN] = true;
		
		// SetRelays control command for gain relays
		ControlSetRelays *controlSetRelays = static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS]);
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
	/// \return The offset that has been set, ::Dso::ErrorCode on error.
	double Control::setOffset(unsigned int channel, double offset) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(channel >= HANTEK_CHANNELS)
			return Dso::ERROR_PARAMETER;
		
		// Calculate the offset value
		// The range is given by the calibration data (convert from big endian)
		unsigned short int minimum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_START]) + 1);
		unsigned short int maximum = ((unsigned short int) *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END])) << 8) + *((unsigned char *) &(this->specification.offsetLimit[channel][this->settings.voltage[channel].gain][OFFSET_END]) + 1);
		unsigned short int offsetValue = offset * (maximum - minimum) + minimum + 0.5;
		double offsetReal = (double) (offsetValue - minimum) / (maximum - minimum);
		
		// SetOffset control command for channel offset
		static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setChannel(channel, offsetValue);
		this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		
		this->settings.voltage[channel].offset = offset;
		this->settings.voltage[channel].offsetReal = offsetReal;
		
		this->setTriggerLevel(channel, this->settings.trigger.level[channel]);
		
		return offsetReal;
	}
	
	/// \brief Set the trigger mode.
	/// \return See ::Dso::ErrorCode.
	int Control::setTriggerMode(Dso::TriggerMode mode) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(mode < Dso::TRIGGERMODE_AUTO || mode > Dso::TRIGGERMODE_SINGLE)
			return Dso::ERROR_PARAMETER;
		
		this->settings.trigger.mode = mode;
		return Dso::ERROR_NONE;
	}
	
	/// \brief Set the trigger source.
	/// \param special true for a special channel (EXT, ...) as trigger source.
	/// \param id The number of the channel, that should be used as trigger.
	/// \return See ::Dso::ErrorCode.
	int Control::setTriggerSource(bool special, unsigned int id) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if((!special && id >= HANTEK_CHANNELS) || (special && id >= HANTEK_SPECIAL_CHANNELS))
			return Dso::ERROR_PARAMETER;
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGERANDSAMPLERATE:
				// SetTriggerAndSamplerate bulk command for trigger source
				static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerSource(special ? 3 + id : 1 - id);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
			
			case BULK_CSETTRIGGERORSAMPLERATE:
				// SetTrigger2250 bulk command for trigger source
				static_cast<BulkSetTrigger2250 *>(this->command[BULK_CSETTRIGGERORSAMPLERATE])->setTriggerSource(special ? 0 : 2 + id);
				this->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
				break;
			
			case BULK_ESETTRIGGERORSAMPLERATE:
				// SetTrigger5200 bulk command for trigger source
				static_cast<BulkSetTrigger5200 *>(this->command[BULK_ESETTRIGGERORSAMPLERATE])->setTriggerSource(special ? 3 + id : 1 - id);
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				break;
			
			default:
				return Dso::ERROR_UNSUPPORTED;
		}
		
		// SetRelays control command for external trigger relay
		static_cast<ControlSetRelays *>(this->control[CONTROLINDEX_SETRELAYS])->setTrigger(special);
		this->controlPending[CONTROLINDEX_SETRELAYS] = true;
		
		this->settings.trigger.special = special;
		this->settings.trigger.source = id;
		
		// Apply trigger level of the new source
		if(special) {
			// SetOffset control command for changed trigger level
			static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setTrigger(0x7f);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		else
			this->setTriggerLevel(id, this->settings.trigger.level[id]);
		
		return Dso::ERROR_NONE;
	}
	
	/// \brief Set the trigger level.
	/// \param channel The channel that should be set.
	/// \param level The new trigger level (V).
	/// \return The trigger level that has been set, ::Dso::ErrorCode on error.
	double Control::setTriggerLevel(unsigned int channel, double level) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(channel >= HANTEK_CHANNELS)
			return Dso::ERROR_PARAMETER;
		
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
			static_cast<ControlSetOffset *>(this->control[CONTROLINDEX_SETOFFSET])->setTrigger(levelValue);
			this->controlPending[CONTROLINDEX_SETOFFSET] = true;
		}
		
		/// \todo Get alternating trigger in here
		
		this->settings.trigger.level[channel] = level;
		return (double) ((levelValue - minimum) / (maximum - minimum) - this->settings.voltage[channel].offsetReal) * this->specification.gainSteps[this->settings.voltage[channel].gain];
	}
	
	/// \brief Set the trigger slope.
	/// \param slope The Slope that should cause a trigger.
	/// \return See ::Dso::ErrorCode.
	int Control::setTriggerSlope(Dso::Slope slope) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		if(slope != Dso::SLOPE_NEGATIVE && slope != Dso::SLOPE_POSITIVE)
			return Dso::ERROR_PARAMETER;
		
		switch(this->specification.command.bulk.setTrigger) {
			case BULK_SETTRIGGERANDSAMPLERATE: {
				// SetTriggerAndSamplerate bulk command for trigger slope
				static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerSlope(slope);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				break;
			}
			case BULK_CSETTRIGGERORSAMPLERATE: {
				// SetTrigger2250 bulk command for trigger slope
				static_cast<BulkSetTrigger2250 *>(this->command[BULK_CSETTRIGGERORSAMPLERATE])->setTriggerSlope(slope);
				this->commandPending[BULK_CSETTRIGGERORSAMPLERATE] = true;
				break;
			}
			case BULK_ESETTRIGGERORSAMPLERATE: {
				// SetTrigger5200 bulk command for trigger slope
				static_cast<BulkSetTrigger5200 *>(this->command[BULK_ESETTRIGGERORSAMPLERATE])->setTriggerSlope(slope);
				this->commandPending[BULK_ESETTRIGGERORSAMPLERATE] = true;
				break;
			}
			default:
				return Dso::ERROR_UNSUPPORTED;
		}
		
		this->settings.trigger.slope = slope;
		return Dso::ERROR_NONE;
	}
	
	/// \brief Set the trigger position.
	/// \param position The new trigger position (in s).
	/// \return The trigger position that has been set.
	double Control::setPretriggerPosition(double position) {
		if(!this->device->isConnected())
			return -2;
		
		// All trigger positions are measured in samples
		unsigned long int positionSamples = position * this->settings.samplerate.current;
		// Fast rate mode uses both channels
		if(this->settings.samplerate.limits == &this->specification.samplerate.multi)
			positionSamples /= HANTEK_CHANNELS;
		
		switch(this->specification.command.bulk.setPretrigger) {
			case BULK_SETTRIGGERANDSAMPLERATE: {
				// Calculate the position value (Start point depending on record length)
				unsigned long int position = 0x7ffff - this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] + positionSamples;
				
				// SetTriggerAndSamplerate bulk command for trigger position
				static_cast<BulkSetTriggerAndSamplerate *>(this->command[BULK_SETTRIGGERANDSAMPLERATE])->setTriggerPosition(position);
				this->commandPending[BULK_SETTRIGGERANDSAMPLERATE] = true;
				
				break;
			}
			case BULK_FSETBUFFER: {
				// Calculate the position values (Inverse, maximum is 0x7ffff)
				unsigned long int positionPre = 0x7fffful - this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] + positionSamples;
				unsigned long int positionPost = 0x7fffful - positionSamples;
				
				// SetBuffer2250 bulk command for trigger position
				BulkSetBuffer2250 *commandSetBuffer2250 = static_cast<BulkSetBuffer2250 *>(this->command[BULK_FSETBUFFER]);
				commandSetBuffer2250->setTriggerPositionPre(positionPre);
				commandSetBuffer2250->setTriggerPositionPost(positionPost);
				this->commandPending[BULK_FSETBUFFER] = true;
				
				break;
			}
			case BULK_ESETTRIGGERORSAMPLERATE: {
				// Calculate the position values (Inverse, maximum is 0xffff)
				unsigned short int positionPre = 0xffff - this->specification.samplerate.single.recordLengths[this->settings.recordLengthId] + positionSamples;
				unsigned short int positionPost = 0xffff - positionSamples;
				
				// SetBuffer5200 bulk command for trigger position
				BulkSetBuffer5200 *commandSetBuffer5200 = static_cast<BulkSetBuffer5200 *>(this->command[BULK_DSETBUFFER]);
				commandSetBuffer5200->setTriggerPositionPre(positionPre);
				commandSetBuffer5200->setTriggerPositionPost(positionPost);
				this->commandPending[BULK_DSETBUFFER] = true;
				
				break;
			}
			default:
				return Dso::ERROR_UNSUPPORTED;
		}
		
		this->settings.trigger.position = position;
		return (double) positionSamples / this->settings.samplerate.current;
	}
	
#ifdef DEBUG
	/// \brief Sends bulk/control commands directly.
	/// <p>
	///		<b>Syntax:</b><br />
	///		<br />
	///		Bulk command:
	///		<pre>send bulk [<em>hex data</em>]</pre>
	///		%Control command:
	///		<pre>send control [<em>hex code</em>] [<em>hex data</em>]</pre>
	/// </p>
	/// \param command The command as string (Has to be parsed).
	/// \return See ::Dso::ErrorCode.
	int Control::stringCommand(QString command) {
		if(!this->device->isConnected())
			return Dso::ERROR_CONNECTION;
		
		QStringList commandParts = command.split(' ', QString::SkipEmptyParts);
		
		if(commandParts.count() >= 1) {
			if(commandParts[0] == "send") {
				if(commandParts.count() >= 2) {
					if(commandParts[1] == "bulk") {
						QString data = command.section(' ', 2, -1, QString::SectionSkipEmpty);
						unsigned char commandCode = 0;
						
						// Read command code (First byte)
						Helper::hexParse(commandParts[2], &commandCode, 1);
						if(commandCode > BULK_COUNT)
							return Dso::ERROR_UNSUPPORTED;
						
						// Update bulk command and mark as pending
						Helper::hexParse(data, this->command[commandCode]->data(), this->command[commandCode]->getSize());
						this->commandPending[commandCode] = true;
						return Dso::ERROR_NONE;
					}
					else if(commandParts[1] == "control") {
						unsigned char controlCode = 0;
						
						// Read command code (First byte)
						Helper::hexParse(commandParts[2], &controlCode, 1);
						int control;
						for(control = 0; control < CONTROLINDEX_COUNT; ++control) {
							if(this->controlCode[control] == controlCode)
								break;
						}
						if(control >= CONTROLINDEX_COUNT)
							return Dso::ERROR_UNSUPPORTED;
						
						QString data = command.section(' ', 3, -1, QString::SectionSkipEmpty);
						
						// Update control command and mark as pending
						Helper::hexParse(data, this->control[control]->data(), this->control[control]->getSize());
						this->controlPending[control] = true;
						return Dso::ERROR_NONE;
					}
				}
				else {
					return Dso::ERROR_PARAMETER;
				}
			}
		}
		else {
			return Dso::ERROR_PARAMETER;
		}
		
		return Dso::ERROR_UNSUPPORTED;
	}
#endif
}
