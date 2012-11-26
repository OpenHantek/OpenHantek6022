////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dockwindows.h
/// \brief Declares the docking window classes.
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


#ifndef DOCKWINDOWS_H
#define DOCKWINDOWS_H


#include <QDockWidget>
#include <QGridLayout>


#include "dso.h"
#include "settings.h"


class QLabel;
class QCheckBox;
class QComboBox;

class SiSpinBox;


////////////////////////////////////////////////////////////////////////////////
/// \class HorizontalDock                                          dockwindows.h
/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
	Q_OBJECT
	
	public:
		HorizontalDock(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~HorizontalDock();
		
		void setFrequencybase(double timebase);
		void setSamplerate(double samplerate);
		void setTimebase(double timebase);
		void setRecordLength(unsigned int recordLength);
		int setFormat(Dso::GraphFormat format);
	
	protected:
		void closeEvent(QCloseEvent *event);
		
		QGridLayout *dockLayout; ///< The main layout for the dock window
		QWidget *dockWidget; ///< The main widget for the dock window
		QLabel *samplerateLabel; ///< The label for the samplerate spinbox
		QLabel *timebaseLabel; ///< The label for the timebase spinbox
		QLabel *frequencybaseLabel; ///< The label for the frequencybase spinbox
		QLabel *recordLengthLabel; ///< The label for the record length combobox
		QLabel *formatLabel; ///< The label for the format combobox
		SiSpinBox *samplerateSiSpinBox; ///< Selects the samplerate for aquisitions
		SiSpinBox *timebaseSiSpinBox; ///< Selects the timebase for voltage graphs
		SiSpinBox *frequencybaseSiSpinBox; ///< Selects the frequencybase for spectrum graphs
		QComboBox *recordLengthComboBox; ///< Selects the record length for aquisitions
		QComboBox *formatComboBox; ///< Selects the way the sampled data is interpreted and shown
		
		DsoSettings *settings; ///< The settings provided by the parent class
		
		QStringList formatStrings; ///< Strings for the formats
		
		bool suppressSignals; ///< Disable changed-signals temporarily
	
	public slots:
		void availableRecordLengthsChanged(const QList<unsigned int> &recordLengths);
		void samplerateLimitsChanged(double minimum, double maximum);
		
	protected slots:
		void frequencybaseSelected(double frequencybase);
		void samplerateSelected(double samplerate);
		void timebaseSelected(double timebase);
		void recordLengthSelected(int index);
		void formatSelected(int index);
	
	signals:
		void frequencybaseChanged(double frequencybase); ///< The frequencybase has been changed
		void samplerateChanged(double samplerate); ///< The samplerate has been changed
		void timebaseChanged(double timebase); ///< The timebase has been changed
		void recordLengthChanged(unsigned long recordLength); ///< The recordd length has been changed
		void formatChanged(Dso::GraphFormat format); ///< The viewing format has been changed
};


////////////////////////////////////////////////////////////////////////////////
/// \class TriggerDock                                             dockwindows.h
/// \brief Dock window for the trigger settings.
/// It contains the settings for the trigger mode, source and slope.
class TriggerDock : public QDockWidget {
	Q_OBJECT
	
	public:
		TriggerDock(DsoSettings *settings, const QStringList *specialTriggers, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~TriggerDock();
		
		int setMode(Dso::TriggerMode mode);
		int setSource(bool special, unsigned int id);
		int setSlope(Dso::Slope slope);
	
	protected:
		void closeEvent(QCloseEvent *event);
		
		QGridLayout *dockLayout; ///< The main layout for the dock window
		QWidget *dockWidget; ///< The main widget for the dock window
		QLabel *modeLabel; ///< The label for the trigger mode combobox
		QLabel *sourceLabel; ///< The label for the trigger source combobox
		QLabel *slopeLabel; ///< The label for the trigger slope combobox
		QComboBox *modeComboBox; ///< Select the triggering mode
		QComboBox *sourceComboBox; ///< Select the source for triggering
		QComboBox *slopeComboBox; ///< Select the slope that causes triggering
		
		DsoSettings *settings; ///< The settings provided by the parent class
		
		QStringList modeStrings; ///< Strings for the trigger modes
		QStringList sourceStandardStrings; ///< Strings for the standard trigger sources
		QStringList sourceSpecialStrings; ///< Strings for the special trigger sources
		QStringList slopeStrings; ///< Strings for the trigger slopes
	
	protected slots:
		void modeSelected(int index);
		void slopeSelected(int index);
		void sourceSelected(int index);
	
	signals:
		void modeChanged(Dso::TriggerMode); ///< The trigger mode has been changed
		void sourceChanged(bool special, unsigned int id); ///< The trigger source has been changed
		void slopeChanged(Dso::Slope); ///< The trigger slope has been changed
};


////////////////////////////////////////////////////////////////////////////////
/// \class VoltageDock                                             dockwindows.h
/// \brief Dock window for the voltage channel settings.
/// It contains the settings for gain and coupling for both channels and
/// allows to enable/disable the channels.
class VoltageDock : public QDockWidget {
	Q_OBJECT
	
	public:
		VoltageDock(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~VoltageDock();
		
		int setCoupling(int channel, Dso::Coupling coupling);
		int setGain(int channel, double gain);
		int setMode(Dso::MathMode mode);
		int setUsed(int channel, bool used);
	
	protected:
		void closeEvent(QCloseEvent *event);
		
		QGridLayout *dockLayout; ///< The main layout for the dock window
		QWidget *dockWidget; ///< The main widget for the dock window
		QList<QCheckBox *> usedCheckBox; ///< Enable/disable a specific channel
		QList<QComboBox *> gainComboBox; ///< Select the vertical gain for the channels
		QList<QComboBox *> miscComboBox; ///< Select coupling for real and mode for math channels
		
		DsoSettings *settings; ///< The settings provided by the parent class
		
		QStringList couplingStrings; ///< The strings for the couplings
		QStringList modeStrings; ///< The strings for the math mode
		QList<double> gainSteps; ///< The selectable gain steps
		QStringList gainStrings; ///< String representations for the gain steps
	
	protected slots:
		void gainSelected(int index);
		void miscSelected(int index);
		void usedSwitched(bool checked);
	
	signals:
		void couplingChanged(unsigned int channel, Dso::Coupling coupling); ///< A coupling has been selected
		void gainChanged(unsigned int channel, double gain); ///< A gain has been selected
		void modeChanged(Dso::MathMode mode); ///< The mode for the math channels has been changed
		void usedChanged(unsigned int channel, bool used); ///< A channel has been enabled/disabled
};


////////////////////////////////////////////////////////////////////////////////
/// \class SpectrumDock                                            dockwindows.h
/// \brief Dock window for the spectrum view.
/// It contains the magnitude for all channels and allows to enable/disable the
/// channels.
class SpectrumDock : public QDockWidget {
	Q_OBJECT
	
	public:
		SpectrumDock(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~SpectrumDock();
		
		int setMagnitude(int channel, double magnitude);
		int setUsed(int channel, bool used);
	
	protected:
		void closeEvent(QCloseEvent *event);
		
		QGridLayout *dockLayout; ///< The main layout for the dock window
		QWidget *dockWidget; ///< The main widget for the dock window
		QList<QCheckBox *> usedCheckBox; ///< Enable/disable spectrum for a channel
		QList<QComboBox *> magnitudeComboBox; ///< Select the vertical magnitude for the spectrums
		
		DsoSettings *settings; ///< The settings provided by the parent class
		
		QList<double> magnitudeSteps; ///< The selectable magnitude steps
		QStringList magnitudeStrings; ///< String representations for the magnitude steps
	
	public slots:
		void magnitudeSelected(int index);
		void usedSwitched(bool checked);
	
	signals:
		void magnitudeChanged(unsigned int channel, double magnitude); ///< A magnitude has been selected
		void usedChanged(unsigned int channel, bool used); ///< A spectrum has been enabled/disabled
};


#endif
