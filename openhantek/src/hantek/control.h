////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file hantek/control.h
/// \brief Declares the Hantek::Control class.
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


#ifndef HANTEK_CONTROL_H
#define HANTEK_CONTROL_H


#include <QMutex>


#include "dsocontrol.h"
#include "helper.h"
#include "hantek/types.h"


namespace Hantek {
	class Device;
	
	//////////////////////////////////////////////////////////////////////////////
	/// \enum ControlIndex                                        hantek/control.h
	/// \brief The array indices for the waiting control commands.
	enum ControlIndex {
		//CONTROLINDEX_VALUE,
		//CONTROLINDEX_GETSPEED,
		//CONTROLINDEX_BEGINCOMMAND,
		CONTROLINDEX_SETOFFSET,
		CONTROLINDEX_SETRELAYS,
		CONTROLINDEX_COUNT
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \class Control                                            hantek/control.h
	/// \brief The DsoControl abstraction layer for %Hantek USB DSOs.
	class Control : public DsoControl {
		Q_OBJECT
		
		public:
			Control(QObject *parent = 0);
			~Control();
			
			unsigned int getChannelCount();
		
		protected:
			void run();
			
			unsigned int calculateTriggerPoint(unsigned int value);
			int getCaptureState();
			int getSamples(bool process);
			
			Device *device; ///< The USB device for the oscilloscope
			
			Helper::DataArray<unsigned char> *command[COMMAND_COUNT]; ///< Pointers to commands, ready to be transmitted
			bool commandPending[COMMAND_COUNT]; ///< true, when the command should be executed
			Helper::DataArray<unsigned char> *control[CONTROLINDEX_COUNT]; ///< Pointers to control commands
			unsigned char controlCode[CONTROLINDEX_COUNT]; ///< Request codes for control commands
			bool controlPending[CONTROLINDEX_COUNT]; ///< true, when the control command should be executed
			
			/// Calibration data for the channel offsets
			unsigned short channelLevels[HANTEK_CHANNELS][GAIN_COUNT][OFFSET_COUNT];
			
			// Various cached settings
			Samplerate samplerate; ///< The samplerate id
			Gain gain[HANTEK_CHANNELS]; ///< The gain id
			double offset[HANTEK_CHANNELS]; ///< The current screen offset for each channel
			double offsetReal[HANTEK_CHANNELS]; ///< The real offset for each channel (Due to quantization)
			double triggerLevel[HANTEK_CHANNELS]; ///< The trigger level for each channel in V
			unsigned int bufferSize; ///< The buffer size in samples
			unsigned int triggerPoint; ///< The trigger point value
			Dso::TriggerMode triggerMode; ///< The trigger mode
			bool triggerSpecial; ///< true, if the trigger source is special
			unsigned int triggerSource; ///< The trigger source
			
			QList<double *> samples; ///< Sample data arrays
			QList<unsigned int> samplesSize; ///< Number of samples data array
			QMutex samplesMutex; ///< Mutex for the sample data
			
			// Lists for enums
			QList<double> gainSteps; ///< Voltage steps in V/screenheight
			QList<unsigned long int> samplerateSteps; ///< Samplerate steps in S/s
			QList<unsigned short int> samplerateValues; ///< Values sent to the oscilloscope
		
		public slots:
			unsigned long int setSamplerate(unsigned long int samplerate);
			double setBufferSize(unsigned int size);
			
			int setChannelUsed(unsigned int channel, bool used);
			int setCoupling(unsigned int channel, Dso::Coupling coupling);
			double setGain(unsigned int channel, double gain);
			double setOffset(unsigned int channel, double offset);
			
			int setTriggerMode(Dso::TriggerMode mode);
			int setTriggerSource(bool special, unsigned int id);
			double setTriggerLevel(unsigned int channel, double level);
			int setTriggerSlope(Dso::Slope slope);
			double setTriggerPosition(double position);
	};
}


#endif
