////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dsocontrol.h
/// \brief Declares the abstract Control class.
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


#ifndef DSOCONTROL_H
#define DSOCONTROL_H


#include <QStringList>
#include <QThread>


#include "constants.h"
#include "helper.h"


class QMutex;


/// \class DsoControl
/// \brief A abstraction layer that enables protocol-independent dso usage.
class DsoControl : public QThread {
	Q_OBJECT
	
	public:
		DsoControl(QObject *parent = 0);
		
		virtual void startSampling();
		virtual void stopSampling();
		
		virtual unsigned int getChannelCount() = 0;
		
		const QStringList *getSpecialTriggerSources();
	
	protected:
		bool sampling; ///< true, if the oscilloscope is taking samples
		bool terminate; ///< true, if the thread should be terminated
		
		QStringList specialTriggerSources; ///< Names of the special trigger sources
		
	signals:
		void deviceConnected();
		void deviceDisconnected();
		void statusMessage(const QString &message, int timeout);
		void samplesAvailable(const QList<double *> *data, const QList<unsigned int> *size, double samplerate, QMutex *mutex);
	
	public slots:
		virtual void connectDevice();
		virtual void disconnectDevice();
		
		virtual unsigned long int setSamplerate(unsigned long int samplerate) = 0;
		virtual double setBufferSize(unsigned int size) = 0;
		
		virtual int setTriggerMode(Dso::TriggerMode mode) = 0;
		virtual int setTriggerSource(bool special, unsigned int id) = 0;
		virtual double setTriggerLevel(unsigned int channel, double level) = 0;
		virtual int setTriggerSlope(Dso::Slope slope) = 0;
		virtual double setTriggerPosition(double position) = 0;
		
		virtual int setChannelUsed(unsigned int channel, bool used) = 0;
		virtual int setCoupling(unsigned int channel, Dso::Coupling coupling) = 0;
		virtual double setGain(unsigned int channel, double gain) = 0;
		virtual double setOffset(unsigned int channel, double offset) = 0;
};


#endif
