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


#include "constants.h"
#include "settings.h"


class QLabel;
class QCheckBox;
class QComboBox;


////////////////////////////////////////////////////////////////////////////////
/// \class HorizontalDock                                          dockwindows.h
/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
	Q_OBJECT
	
	public:
		HorizontalDock(DsoSettings *settings, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~HorizontalDock();
		
		int setFrequencybase(double timebase);
		int setTimebase(double timebase);
		int setFormat(Dso::GraphFormat format);
	
	protected:
		void closeEvent(QCloseEvent *event);
		
		QGridLayout *dockLayout;
		QWidget *dockWidget;
		QLabel *timebaseLabel, *frequencybaseLabel, *formatLabel;
		QComboBox *timebaseComboBox, *frequencybaseComboBox, *formatComboBox;
		
		DsoSettings *settings;
		
		QList<double> frequencybaseSteps, timebaseSteps;
		QStringList frequencybaseStrings, timebaseStrings, formatStrings;
	
	protected slots:
		void frequencybaseSelected(int index);
		void timebaseSelected(int index);
		void formatSelected(int index);
	
	signals:
		void frequencybaseChanged(double frequencybase);
		void timebaseChanged(double timebase);
		void formatChanged(Dso::GraphFormat format);
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
		
		QGridLayout *dockLayout;
		QWidget *dockWidget;
		QLabel *modeLabel, *sweepLabel, *sourceLabel, *slopeLabel;
		QComboBox *modeComboBox, *sweepComboBox, *sourceComboBox, *slopeComboBox;
		
		DsoSettings *settings;
		
		QStringList modeStrings, sourceStandardStrings, sourceSpecialStrings, slopeStrings;
	
	protected slots:
		void modeSelected(int index);
		void slopeSelected(int index);
		void sourceSelected(int index);
	
	signals:
		void modeChanged(Dso::TriggerMode);
		void sourceChanged(bool special, unsigned int id);
		void slopeChanged(Dso::Slope);
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
		
		QGridLayout *dockLayout;
		QWidget *dockWidget;
		QList<QCheckBox *> usedCheckBox;
		QList<QComboBox *> gainComboBox, miscComboBox;
		
		DsoSettings *settings;
		
		QStringList couplingStrings;
		QStringList modeStrings;
		QList<double> gainSteps;
		QStringList gainStrings;
	
	protected slots:
		void gainSelected(int index);
		void miscSelected(int index);
		void usedSwitched(bool checked);
	
	signals:
		void couplingChanged(unsigned int channel, Dso::Coupling coupling);
		void gainChanged(unsigned int channel, double gain);
		void modeChanged(Dso::MathMode mode);
		void usedChanged(unsigned int channel, bool used);
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
		
		QGridLayout *dockLayout;
		QWidget *dockWidget;
		QList<QCheckBox *> usedCheckBox;
		QList<QComboBox *> magnitudeComboBox;
		
		DsoSettings *settings;
		
		QList<double> magnitudeSteps;
		QStringList magnitudeStrings;
	
	public slots:
		void magnitudeSelected(int index);
		void usedSwitched(bool checked);
	
	signals:
		void magnitudeChanged(unsigned int channel, double magnitude);
		void usedChanged(unsigned int channel, bool used);
};


#endif
