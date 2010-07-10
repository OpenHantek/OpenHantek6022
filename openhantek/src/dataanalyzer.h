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


#include <QThread>


#include "constants.h"
#include "helper.h"


class DsoSettings;
class HantekDSOAThread;
class QMutex;


////////////////////////////////////////////////////////////////////////////////
/// \struct SampleValues                                          dataanalyzer.h
/// \brief Struct for a array of sample values.
struct SampleValues {
	double *sample;
	unsigned int count;
	double interval;
};

////////////////////////////////////////////////////////////////////////////////
/// \struct SampleData                                            dataanalyzer.h
/// \brief Struct for the sample value arrays.
struct SampleData {
	SampleValues voltage, spectrum;
};

////////////////////////////////////////////////////////////////////////////////
/// \struct AnalyzedData                                          dataanalyzer.h
/// \brief Struct for the analyzed data.
struct AnalyzedData {
	SampleData samples;
	double frequency, amplitude;
};

////////////////////////////////////////////////////////////////////////////////
/// \class DataAnalyzer                                           dataanalyzer.h
/// \brief Analyzes the data from the dso.
/// Converts the levels into volts, calculates the spectrum and saves offsets
/// and the time-/frequencysteps between two values.
class DataAnalyzer : public QThread {
	Q_OBJECT
	
	public:
		DataAnalyzer(DsoSettings *settings, QObject *parent = 0);
		~DataAnalyzer();
		
		const AnalyzedData *data(int channel) const;
		QMutex *mutex() const;
	
	protected:
		void run();
	
	private:
		DsoSettings *settings;
		
		QList<AnalyzedData *> analyzedData;
		QMutex *analyzedDataMutex;
		
		unsigned int lastBufferSize;
		Dso::WindowFunction lastWindow;
		double *window;
		
		QList<double *> waitingData;
		QList<unsigned int> waitingDataSize;
		double waitingDataSamplerate;
		QMutex *waitingDataMutex;
	
	public slots:
		void analyze(const QList<double *> *data, const QList<unsigned int> *size, double samplerate, QMutex *mutex);
};

#endif
