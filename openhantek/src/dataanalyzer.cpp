////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
//  dataanalyzer.cpp
//
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


#include <cmath>

#include <QColor>
#include <QMutex>

#include <fftw3.h>


#include "dataanalyzer.h"

#include "glscope.h"
#include "helper.h"
#include "settings.h"


////////////////////////////////////////////////////////////////////////////////
// struct SampleValues
/// \brief Initializes the members to their default values.
SampleValues::SampleValues() {
	this->interval = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// struct AnalyzedData
/// \brief Initializes the members to their default values.
AnalyzedData::AnalyzedData() {
	this->amplitude = 0.0;
	this->frequency = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// class DataAnalyzer
/// \brief Initializes the buffers and other variables.
/// \param settings The settings that should be used.
/// \param parent The parent widget.
DataAnalyzer::DataAnalyzer(DsoSettings *settings, QObject *parent) : QThread(parent) {
	this->settings = settings;
	
	this->lastRecordLength = 0;
	this->lastWindow = (Dso::WindowFunction) -1;
	this->window = 0;
	
	this->analyzedDataMutex = new QMutex();
	
	this->maxSamples = 0;
	
	this->waitingDataSamplerate = 0.0;
	this->waitingDataMutex = 0;
}

/// \brief Deallocates the buffers.
DataAnalyzer::~DataAnalyzer() {
}

/// \brief Returns the analyzed data.
/// \param channel Channel, whose data should be returned.
/// \return Analyzed data as AnalyzedData struct.
AnalyzedData const *DataAnalyzer::data(unsigned int channel) const {
	if(channel >= this->analyzedData.size())
		return 0;
	
	return &this->analyzedData[channel];
}

/// \brief Returns the sample count of the analyzed data.
/// \return The maximum sample count of the last analyzed data.
unsigned int DataAnalyzer::sampleCount() {
	return this->maxSamples;
}

/// \brief Returns the mutex for the data.
/// \return Mutex for the analyzed data.
QMutex *DataAnalyzer::mutex() const {
	return this->analyzedDataMutex;
}

/// \brief Analyzes the data from the dso.
void DataAnalyzer::run() {
	this->analyzedDataMutex->lock();
	
	unsigned int maxSamples = 0;
	unsigned int channelCount = (unsigned int) this->settings->scope.voltage.size();
	
	// Adapt the number of channels for analyzed data
	this->analyzedData.resize(channelCount);
	
	for(unsigned int channel = 0; channel < channelCount; ++channel) {
		 AnalyzedData *const channelData = &this->analyzedData[channel];
		
		if( // Check...
			( // ...if we got data for this channel...
				channel < this->settings->scope.physicalChannels &&
				channel < (unsigned int) this->waitingData->size() &&
				!this->waitingData->at(channel).empty()) ||
			( // ...or if it's a math channel that can be calculated
				channel >= this->settings->scope.physicalChannels &&
				(this->settings->scope.voltage[channel].used || this->settings->scope.spectrum[channel].used) &&
				this->analyzedData.size() >= 2 &&
				!this->analyzedData[0].samples.voltage.sample.empty() &&
				!this->analyzedData[1].samples.voltage.sample.empty()
			)
		) {
			// Set sampling interval
			const double interval = 1.0 / this->waitingDataSamplerate;
			if(interval != channelData->samples.voltage.interval) {
				channelData->samples.voltage.interval = interval;
				if(this->waitingDataAppend) // Clear roll buffer if the samplerate changed
					channelData->samples.voltage.sample.clear();
			}

			
			unsigned int size;
			if(channel < this->settings->scope.physicalChannels) {
				size = this->waitingData->at(channel).size();
				if(this->waitingDataAppend)
					size += channelData->samples.voltage.sample.size();
				if(size > maxSamples)
					maxSamples = size;
			}
			else
				size = maxSamples;
			
			// Physical channels
			if(channel < this->settings->scope.physicalChannels) {
				// Copy the buffer of the oscilloscope into the sample buffer
				if(this->waitingDataAppend)
					channelData->samples.voltage.sample.insert(channelData->samples.voltage.sample.end(), this->waitingData->at(channel).begin(), this->waitingData->at(channel).end());
				else
					channelData->samples.voltage.sample = this->waitingData->at(channel);
			}
			// Math channel
			else {
				// Resize the sample vector
				channelData->samples.voltage.sample.resize(size);
				// Set sampling interval
				this->analyzedData[this->settings->scope.physicalChannels].samples.voltage.interval = this->analyzedData[0].samples.voltage.interval;
				
				// Resize the sample vector
				this->analyzedData[this->settings->scope.physicalChannels].samples.voltage.sample.resize(qMin(this->analyzedData[0].samples.voltage.sample.size(), this->analyzedData[1].samples.voltage.sample.size()));
				
				// Calculate values and write them into the sample buffer
				std::vector<double>::const_iterator ch1Iterator = this->analyzedData[0].samples.voltage.sample.begin();
				std::vector<double>::const_iterator ch2Iterator = this->analyzedData[1].samples.voltage.sample.begin();
				std::vector<double> &resultData = this->analyzedData[this->settings->scope.physicalChannels].samples.voltage.sample;
				for(std::vector<double>::iterator resultIterator = resultData.begin(); resultIterator != resultData.end(); ++resultIterator) {
					switch(this->settings->scope.voltage[this->settings->scope.physicalChannels].misc) {
						case Dso::MATHMODE_1ADD2:
							*(resultIterator++) = *(ch1Iterator++) + *(ch2Iterator++);
							break;
						case Dso::MATHMODE_1SUB2:
							*(resultIterator++) = *(ch1Iterator++) - *(ch2Iterator++);
							break;
						case Dso::MATHMODE_2SUB1:
							*(resultIterator++) = *(ch2Iterator++) - *(ch1Iterator++);
							break;
					}
				}
			}
		}
		else {
			// Clear unused channels
			channelData->samples.voltage.sample.clear();
			this->analyzedData[this->settings->scope.physicalChannels].samples.voltage.interval = 0;
		}
	}
	
	this->waitingDataMutex->unlock();
	
	// Lower priority for spectrum calculation
	this->setPriority(QThread::LowPriority);
	
	
	// Calculate frequencies, peak-to-peak voltages and spectrums
	for(unsigned int channel = 0; channel < this->analyzedData.size(); ++channel) {
		AnalyzedData *const channelData = &this->analyzedData[channel];
		
		if(!channelData->samples.voltage.sample.empty()) {
			// Calculate new window
			unsigned int sampleCount = channelData->samples.voltage.sample.size();
			if(this->lastWindow != this->settings->scope.spectrumWindow || this->lastRecordLength != sampleCount) {
				if(this->lastRecordLength != sampleCount) {
					this->lastRecordLength = sampleCount;
					
					if(this->window)
						fftw_free(this->window);
					this->window = (double *) fftw_malloc(sizeof(double) * this->lastRecordLength);
				}
				
				unsigned int windowEnd = this->lastRecordLength - 1;
				this->lastWindow = this->settings->scope.spectrumWindow;
				
				switch(this->settings->scope.spectrumWindow) {
					case Dso::WINDOW_HAMMING:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.54 - 0.46 * cos(2.0 * M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_HANN:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.5 * (1.0 - cos(2.0 * M_PI * windowPosition / windowEnd));
						break;
					case Dso::WINDOW_COSINE:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = sin(M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_LANCZOS:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition) {
							double sincParameter = (2.0 * windowPosition / windowEnd - 1.0) * M_PI;
							if(sincParameter == 0)
								*(this->window + windowPosition) = 1;
							else
								*(this->window + windowPosition) = sin(sincParameter) / sincParameter;
						}
						break;
					case Dso::WINDOW_BARTLETT:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 2.0 / windowEnd * (windowEnd / 2 - abs(windowPosition - windowEnd / 2));
						break;
					case Dso::WINDOW_TRIANGULAR:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 2.0 / this->lastRecordLength * (this->lastRecordLength / 2 - abs(windowPosition - windowEnd / 2));
						break;
					case Dso::WINDOW_GAUSS:
						{
							double sigma = 0.4;
							for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
								*(this->window + windowPosition) = exp(-0.5 * pow(((windowPosition - windowEnd / 2) / (sigma * windowEnd / 2)), 2));
						}
						break;
					case Dso::WINDOW_BARTLETTHANN:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.62 - 0.48 * abs(windowPosition / windowEnd - 0.5) - 0.38 * cos(2.0 * M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_BLACKMAN:
						{
							double alpha = 0.16;
							for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
								*(this->window + windowPosition) = (1 - alpha) / 2 - 0.5 * cos(2.0 * M_PI * windowPosition / windowEnd) + alpha / 2 * cos(4.0 * M_PI * windowPosition / windowEnd);
						}
						break;
					//case WINDOW_KAISER:
						// TODO
						//double alpha = 3.0;
						//for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							//*(this->window + windowPosition) = ;
						//break;
					case Dso::WINDOW_NUTTALL:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.355768 - 0.487396 * cos(2 * M_PI * windowPosition / windowEnd) + 0.144232 * cos(4 * M_PI * windowPosition / windowEnd) - 0.012604 * cos(6 * M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_BLACKMANHARRIS:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.35875 - 0.48829 * cos(2 * M_PI * windowPosition / windowEnd) + 0.14128 * cos(4 * M_PI * windowPosition / windowEnd) - 0.01168 * cos(6 * M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_BLACKMANNUTTALL:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 0.3635819 - 0.4891775 * cos(2 * M_PI * windowPosition / windowEnd) + 0.1365995 * cos(4 * M_PI * windowPosition / windowEnd) - 0.0106411 * cos(6 * M_PI * windowPosition / windowEnd);
						break;
					case Dso::WINDOW_FLATTOP:
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 1.0 - 1.93 * cos(2 * M_PI * windowPosition / windowEnd) + 1.29 * cos(4 * M_PI * windowPosition / windowEnd) - 0.388 * cos(6 * M_PI * windowPosition / windowEnd) + 0.032 * cos(8 * M_PI * windowPosition / windowEnd);
						break;
					default: // Dso::WINDOW_RECTANGULAR
						for(unsigned int windowPosition = 0; windowPosition < this->lastRecordLength; ++windowPosition)
							*(this->window + windowPosition) = 1.0;
				}
			}
			
			// Set sampling interval
			channelData->samples.spectrum.interval = 1.0 / channelData->samples.voltage.interval / sampleCount;
			
			// Number of real/complex samples
			unsigned int dftLength = sampleCount / 2;
			
			// Reallocate memory for samples if the sample count has changed
			channelData->samples.spectrum.sample.resize(sampleCount);
			
			// Create sample buffer and apply window
			double *windowedValues = new double[sampleCount];
			for(unsigned int position = 0; position < sampleCount; ++position)
				windowedValues[position] = this->window[position] * channelData->samples.voltage.sample[position];
			
			// Do discrete real to half-complex transformation
			/// \todo Check if record length is multiple of 2
			/// \todo Reuse plan and use FFTW_MEASURE to get fastest algorithm
			fftw_plan fftPlan = fftw_plan_r2r_1d(sampleCount, windowedValues, &channelData->samples.spectrum.sample.front(), FFTW_R2HC, FFTW_ESTIMATE);
			fftw_execute(fftPlan);
			fftw_destroy_plan(fftPlan);
			
			// Do an autocorrelation to get the frequency of the signal
			double *conjugateComplex = windowedValues; // Reuse the windowedValues buffer
			
			// Real values
			unsigned int position;
			double correctionFactor = 1.0 / dftLength / dftLength;
			conjugateComplex[0] = (channelData->samples.spectrum.sample[0] * channelData->samples.spectrum.sample[0]) * correctionFactor;
			for(position = 1; position < dftLength; ++position)
				conjugateComplex[position] = (channelData->samples.spectrum.sample[position] * channelData->samples.spectrum.sample[position] + channelData->samples.spectrum.sample[sampleCount - position] * channelData->samples.spectrum.sample[sampleCount - position]) * correctionFactor;
			// Complex values, all zero for autocorrelation
			conjugateComplex[dftLength] = (channelData->samples.spectrum.sample[dftLength] * channelData->samples.spectrum.sample[dftLength]) * correctionFactor;
			for(++position; position < sampleCount; ++position)
				conjugateComplex[position] = 0;
			
			// Do half-complex to real inverse transformation
			double *correlation = new double[sampleCount];
			fftPlan = fftw_plan_r2r_1d(sampleCount, conjugateComplex, correlation, FFTW_HC2R, FFTW_ESTIMATE);
			fftw_execute(fftPlan);
			fftw_destroy_plan(fftPlan);
			delete[] conjugateComplex;
			
			// Calculate peak-to-peak voltage
			double minimalVoltage, maximalVoltage;
			minimalVoltage = maximalVoltage = channelData->samples.voltage.sample[0];
			
			for(unsigned int position = 1; position < sampleCount; ++position) {
				if(channelData->samples.voltage.sample[position] < minimalVoltage)
					minimalVoltage = channelData->samples.voltage.sample[position];
				else if(channelData->samples.voltage.sample[position] > maximalVoltage)
					maximalVoltage = channelData->samples.voltage.sample[position];
			}
			
			channelData->amplitude = maximalVoltage - minimalVoltage;
			
			// Get the frequency from the correlation results
			double minimumCorrelation = correlation[0];
			double peakCorrelation = 0;
			unsigned int peakPosition = 0;
			
			for(unsigned int position = 1; position < sampleCount / 2; ++position) {
				if(correlation[position] > peakCorrelation && correlation[position] > minimumCorrelation * 2) {
					peakCorrelation = correlation[position];
					peakPosition = position;
				}
				else if(correlation[position] < minimumCorrelation)
					minimumCorrelation = correlation[position];
			}
			delete[] correlation;
			
			// Calculate the frequency in Hz
			if(peakPosition)
				channelData->frequency = 1.0 / (channelData->samples.voltage.interval * peakPosition);
			else
				channelData->frequency = 0;
			
			// Finally calculate the real spectrum if we want it
			if(this->settings->scope.spectrum[channel].used) {
				// Convert values into dB (Relative to the reference level)
				double offset = 60 - this->settings->scope.spectrumReference - 20 * log10(dftLength);
				double offsetLimit = this->settings->scope.spectrumLimit - this->settings->scope.spectrumReference;
				for(std::vector<double>::iterator spectrumIterator = channelData->samples.spectrum.sample.begin(); spectrumIterator != channelData->samples.spectrum.sample.end(); ++spectrumIterator) {
					double value = 20 * log10(fabs(channelData->samples.spectrum.sample[position])) + offset;
					
					// Check if this value has to be limited
					if(offsetLimit > value)
						value = offsetLimit;
					
					*spectrumIterator = value;
				}
			}
		}
		else if(!channelData->samples.spectrum.sample.empty()) {
			// Clear unused channels
			channelData->samples.spectrum.interval = 0;
			channelData->samples.spectrum.sample.clear();
		}
	}
	
	this->maxSamples = maxSamples;
	emit(analyzed(maxSamples));
	
	this->analyzedDataMutex->unlock();
}

/// \brief Starts the analyzing of new input data.
/// \param data The data arrays with the input data.
/// \param size The sizes of the data arrays.
/// \param samplerate The samplerate for all input data.
/// \param append The data will be appended to the previously analyzed data (Roll mode).
/// \param mutex The mutex for all input data.
void DataAnalyzer::analyze(const std::vector<std::vector<double> > *data, double samplerate, bool append, QMutex *mutex) {
	// Previous analysis still running, drop the new data
	if(this->isRunning()) {
#ifdef DEBUG
		Helper::timestampDebug("Analyzer overload, dropping packets!");
#endif
		return;
	}
	
	// The thread will analyze it, just save the pointers
	mutex->lock();
	this->waitingData = data;
	this->waitingDataAppend = append;
	this->waitingDataMutex = mutex;
	this->waitingDataSamplerate = samplerate;
	this->start();
#ifdef DEBUG
	static unsigned long id = 0;
	++id;
	Helper::timestampDebug(QString("Analyzed packet %1").arg(id));
#endif
}
