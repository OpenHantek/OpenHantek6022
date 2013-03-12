////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dataanalyzer.h
/// \brief Declares the DataAnalyzer class.
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


#ifndef DATAANALYZER_H
#define DATAANALYZER_H


#include <vector>

#include <QThread>


#include "dso.h"
#include "helper.h"


class DsoSettings;
class HantekDSOAThread;
class QMutex;


////////////////////////////////////////////////////////////////////////////////
/// \struct SampleValues                                          dataanalyzer.h
/// \brief Struct for a array of sample values.
struct SampleValues {
	std::vector<double> sample; ///< Vector holding the sampling data
	double interval; ///< The interval between two sample values
	
	SampleValues();
};

////////////////////////////////////////////////////////////////////////////////
/// \struct SampleData                                            dataanalyzer.h
/// \brief Struct for the sample value arrays.
struct SampleData {
	SampleValues voltage; ///< The time-domain voltage levels (V)
	SampleValues spectrum; ///< The frequency-domain power levels (dB)
};

////////////////////////////////////////////////////////////////////////////////
/// \struct AnalyzedData                                          dataanalyzer.h
/// \brief Struct for the analyzed data.
struct AnalyzedData {
	SampleData samples; ///< Voltage and spectrum values
	double amplitude; ///< The amplitude of the signal
	double frequency; ///< The frequency of the signal
	
	AnalyzedData();
};

////////////////////////////////////////////////////////////////////////////////
/// \class DataAnalyzer                                           dataanalyzer.h
/// \brief Analyzes the data from the dso.
/// Calculates the spectrum and various data about the signal and saves the
/// time-/frequencysteps between two values.
class DataAnalyzer : public QThread {
	Q_OBJECT
	
	public:
		DataAnalyzer(DsoSettings *settings, QObject *parent = 0);
		~DataAnalyzer();
		
		const AnalyzedData *data(unsigned int channel) const;
		unsigned int sampleCount();
		QMutex *mutex() const;
	
	protected:
		void run();
		
		DsoSettings *settings; ///< The settings provided by the parent class
		
		std::vector<AnalyzedData> analyzedData; ///< The analyzed data for each channel
		QMutex *analyzedDataMutex; ///< A mutex for the analyzed data of all channels
		
		unsigned int lastRecordLength; ///< The record length of the previously analyzed data
		unsigned int maxSamples; ///< The maximum record length of the analyzed data
		Dso::WindowFunction lastWindow; ///< The previously used dft window function
		double *window; ///< The array for the dft window factors
		
		const std::vector<std::vector<double> > *waitingData; ///< Pointer to input data from device
		double waitingDataSamplerate; ///< The samplerate of the input data
		bool waitingDataAppend; ///< true, if waiting data should be appended
		QMutex *waitingDataMutex; ///< A mutex for the input data
	
	public slots:
		void analyze(const std::vector<std::vector<double> > *data, double samplerate, bool append, QMutex *mutex);
	
	signals:
		void analyzed(unsigned long samples); ///< The data with that much samples has been analyzed
};

#endif
