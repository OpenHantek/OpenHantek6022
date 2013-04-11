////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file hantek/control.h
/// \brief Declares the Hantek::Control class.
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


#ifndef HANTEK_CONTROL_H
#define HANTEK_CONTROL_H


#include <QMutex>


#include "dsocontrol.h"
#include "helper.h"
#include "hantek/types.h"


class QTimer;


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
	/// \enum RollState                                             hantek/types.h
	/// \brief The states of the roll cycle (Since capture state isn't valid).
	enum RollState {
		ROLL_STARTSAMPLING = 0, ///< Start sampling
		ROLL_ENABLETRIGGER = 1, ///< Enable triggering
		ROLL_FORCETRIGGER = 2, ///< Force triggering
		ROLL_GETDATA = 3, ///< Request sample data
		ROLL_COUNT
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecificationCommandsBulk                  hantek/control.h
	/// \brief Stores the bulk command codes used for this device.
	struct ControlSpecificationCommandsBulk {
		BulkCode setChannels; ///< Command for setting used channels
		BulkCode setSamplerate; ///< Command for samplerate settings
		BulkCode setGain; ///< Command for gain settings (Usually in combination with CONTROL_SETRELAYS)
		BulkCode setRecordLength; ///< Command for buffer settings
		BulkCode setTrigger; ///< Command for trigger settings
		BulkCode setPretrigger; ///< Command for pretrigger settings
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecificationCommandsControl               hantek/control.h
	/// \brief Stores the control command codes used for this device.
	struct ControlSpecificationCommandsControl {
		ControlCode setOffset; ///< Command for setting offset calibration data
		ControlCode setRelays; ///< Command for setting gain relays (Usually in combination with BULK_SETGAIN)
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecificationCommandsValues                hantek/control.h
	/// \brief Stores the control value codes used for this device.
	struct ControlSpecificationCommandsValues {
		ControlValue offsetLimits; ///< Code for channel offset limits
		ControlValue voltageLimits; ///< Code for voltage limits
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecificationCommands                      hantek/control.h
	/// \brief Stores the command codes used for this device.
	struct ControlSpecificationCommands {
		ControlSpecificationCommandsBulk bulk; ///< The used bulk commands
		ControlSpecificationCommandsControl control; ///< The used control commands
		ControlSpecificationCommandsValues values; ///< The used control values
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSamplerateLimits                           hantek/control.h
	/// \brief Stores the samplerate limits for calculations.
	struct ControlSamplerateLimits {
		double base; ///< The base for sample rate calculations
		double max; ///< The maximum sample rate
		unsigned int maxDownsampler; ///< The maximum downsampling ratio
		QList<unsigned int> recordLengths; ///< Available record lengths, UINT_MAX means rolling
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecificationSamplerate                    hantek/control.h
	/// \brief Stores the samplerate limits.
	struct ControlSpecificationSamplerate {
		ControlSamplerateLimits single; ///< The limits for single channel mode
		ControlSamplerateLimits multi; ///< The limits for multi channel mode
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSpecification                              hantek/control.h
	/// \brief Stores the specifications of the currently connected device.
	struct ControlSpecification {
		// Interface
		ControlSpecificationCommands command; ///< The commands for this device
		
		// Limits
		ControlSpecificationSamplerate samplerate; ///< The samplerate specifications
		QList<unsigned int> bufferDividers; ///< Samplerate dividers for record lengths
		QList<double> gainSteps; ///< Available voltage steps in V/screenheight
		unsigned char sampleSize; ///< Number of bits per sample
		
		// Calibration
		/// The sample values at the top of the screen
		QList<unsigned short int> voltageLimit[HANTEK_CHANNELS];
		/// The index of the selected gain on the hardware
		QList<unsigned char> gainIndex;
		/// Calibration data for the channel offsets \todo Should probably be a QList too
		unsigned short int offsetLimit[HANTEK_CHANNELS][9][OFFSET_COUNT];
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSettingsSamplerateTarget                   hantek/control.h
	/// \brief Stores the target samplerate settings of the device.
	struct ControlSettingsSamplerateTarget {
		double samplerate; ///< The target samplerate set via setSamplerate
		double duration; ///< The target record time set via setRecordTime
		bool samplerateSet; ///< true means samplerate was set last, false duration
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSettingsSamplerate                         hantek/control.h
	/// \brief Stores the current samplerate settings of the device.
	struct ControlSettingsSamplerate {
		ControlSettingsSamplerateTarget target; ///< The target samplerate values
		ControlSamplerateLimits *limits; ///< The samplerate limits
		unsigned int downsampler; ///< The variable downsampling factor
		double current; ///< The current samplerate
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSettingsTrigger                            hantek/control.h
	/// \brief Stores the current trigger settings of the device.
	struct ControlSettingsTrigger {
		double level[HANTEK_CHANNELS]; ///< The trigger level for each channel in V
		double position; ///< The current pretrigger position
		unsigned int point; ///< The trigger position in Hantek coding
		Dso::TriggerMode mode; ///< The trigger mode
		Dso::Slope slope; ///< The trigger slope
		bool special; ///< true, if the trigger source is special
		unsigned int source; ///< The trigger source
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSettingsVoltage                            hantek/control.h
	/// \brief Stores the current amplification settings of the device.
	struct ControlSettingsVoltage {
		unsigned int gain; ///< The gain id
		double offset; ///< The screen offset for each channel
		double offsetReal; ///< The real offset for each channel (Due to quantization)
		bool used; ///< true, if the channel is used
	};
	
	//////////////////////////////////////////////////////////////////////////////
	/// \struct ControlSettings                                   hantek/control.h
	/// \brief Stores the current settings of the device.
	struct ControlSettings {
		ControlSettingsSamplerate samplerate; ///< The samplerate settings
		ControlSettingsVoltage voltage[HANTEK_CHANNELS]; ///< The amplification settings
		ControlSettingsTrigger trigger; ///< The trigger settings
		unsigned int recordLengthId; ///< The id in the record length array
		unsigned short int usedChannels; ///< Number of activated channels
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
			QList<unsigned int> *getAvailableRecordLengths();
			double getMinSamplerate();
			double getMaxSamplerate();
		
		protected:
			void run();
			void updateInterval();
			
			unsigned int calculateTriggerPoint(unsigned int value);
			int getCaptureState();
			int getSamples(bool process);
			double getBestSamplerate(double samplerate, bool fastRate = false, bool maximum = false, unsigned int *downsampler = 0);
			unsigned int getSampleCount(bool *fastRate = 0);
			unsigned int updateRecordLength(unsigned int size);
			unsigned int updateSamplerate(unsigned int downsampler, bool fastRate);
			void restoreTargets();
			void updateSamplerateLimits();
			
			// Communication with device
			Device *device; ///< The USB device for the oscilloscope
			QTimer *timer; ///< Timer for periodic communication thread
			
			Helper::DataArray<unsigned char> *command[BULK_COUNT]; ///< Pointers to bulk commands, ready to be transmitted
			bool commandPending[BULK_COUNT]; ///< true, when the command should be executed
			Helper::DataArray<unsigned char> *control[CONTROLINDEX_COUNT]; ///< Pointers to control commands
			unsigned char controlCode[CONTROLINDEX_COUNT]; ///< Request codes for control commands
			bool controlPending[CONTROLINDEX_COUNT]; ///< true, when the control command should be executed
			
			// Device setup
			ControlSpecification specification; ///< The specifications of the device
			ControlSettings settings; ///< The current settings of the device
			
			// Results
			std::vector<std::vector<double> > samples; ///< Sample data vectors sent to the data analyzer
			unsigned int previousSampleCount; ///< The expected total number of samples at the last check before sampling started
			QMutex samplesMutex; ///< Mutex for the sample data
			
			// State of the communication thread
			int captureState;
			int rollState;
			bool samplingStarted;
			Dso::TriggerMode lastTriggerMode;
			int cycleCounter;
			int startCycle;
		
		public slots:
			virtual void connectDevice();
			
			unsigned int setRecordLength(unsigned int size);
			double setSamplerate(double samplerate = 0.0);
			double setRecordTime(double duration = 0.0);
			
			int setChannelUsed(unsigned int channel, bool used);
			int setCoupling(unsigned int channel, Dso::Coupling coupling);
			double setGain(unsigned int channel, double gain);
			double setOffset(unsigned int channel, double offset);
			
			int setTriggerMode(Dso::TriggerMode mode);
			int setTriggerSource(bool special, unsigned int id);
			double setTriggerLevel(unsigned int channel, double level);
			int setTriggerSlope(Dso::Slope slope);
			double setPretriggerPosition(double position);
			
#ifdef DEBUG
			int stringCommand(QString command);
#endif
			
		protected slots:
			void handler();
	};
}


#endif
