////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file dsowidget.h
/// \brief Declares the DsoWidget class.
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


#ifndef DSOWIDGET_H
#define DSOWIDGET_H


#include <QWidget>

#include "dockwindows.h"
#include "glscope.h"
#include "levelslider.h"


class DataAnalyzer;
class DsoSettings;
class QGridLayout;


////////////////////////////////////////////////////////////////////////////////
/// \class DsoWidget                                                 dsowidget.h
/// \brief The widget for the oszilloscope-screen
/// This widget contains the scopes and all level sliders.
class DsoWidget : public QWidget {
	Q_OBJECT
	
	public:
		DsoWidget(DsoSettings *settings, DataAnalyzer *dataAnalyzer, QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~DsoWidget();
	
	protected:
		void adaptTriggerLevelSlider(unsigned int channel);
		void setMeasurementVisible(unsigned int channel, bool visible);
		void updateMarkerDetails();
		void updateSpectrumDetails(unsigned int channel);
		void updateTriggerDetails();
		void updateVoltageDetails(unsigned int channel);
		
		QGridLayout *mainLayout;
		GlGenerator *generator;
		GlScope *mainScope, *zoomScope;
		LevelSlider *offsetSlider;
		LevelSlider *triggerPositionSlider, *triggerLevelSlider;
		LevelSlider *markerSlider;
		
		QHBoxLayout *settingsLayout;
		QLabel *settingsTriggerLabel;
		QLabel *settingsBufferLabel, *settingsRateLabel;
		QLabel *settingsTimebaseLabel, *settingsFrequencybaseLabel;
		
		QHBoxLayout *markerLayout;
		QLabel *markerInfoLabel;
		QLabel *markerTimeLabel, *markerFrequencyLabel;
		QLabel *markerTimebaseLabel, *markerFrequencybaseLabel;
		
		QGridLayout *measurementLayout;
		QList<QLabel *> measurementNameLabel;
		QList<QLabel *> measurementGainLabel, measurementMagnitudeLabel;
		QList<QLabel *> measurementCouplingLabel;
		QList<QLabel *> measurementAmplitudeLabel, measurementFrequencyLabel;
		
		DsoSettings *settings;
		
		DataAnalyzer *dataAnalyzer;
	
	public slots:
		// Horizontal axis
		//void horizontalFormatChanged(HorizontalFormat format);
		void updateFrequencybase();
		void updateSamplerate();
		void updateTimebase();
		
		// Trigger
		void updateTriggerMode();
		void updateTriggerSlope();
		void updateTriggerSource();
		
		// Spectrum
		void updateSpectrumMagnitude(unsigned int channel);
		void updateSpectrumUsed(unsigned int channel, bool used);
		
		// Vertical axis
    void updateVoltageCoupling(unsigned int channel);
		void updateVoltageGain(unsigned int channel);
		void updateVoltageUsed(unsigned int channel, bool used);
		
		// Menus
		void updateBufferSize();
		
		// Export
		bool exportAs();
		bool print();
		
		// Scope control
		void updateZoom(bool enabled);
		
		// Data analyzer
		void dataAnalyzed();
	
	protected slots:
		// Sliders
		void updateOffset(int channel, double value);
		void updateTriggerPosition(int index, double value);
		void updateTriggerLevel(int channel, double value);
		void updateMarker(int marker, double value);
	
	signals:
		// Sliders
		void offsetChanged(unsigned int channel, double value);
		void triggerPositionChanged(double value);
		void triggerLevelChanged(unsigned int channel, double value);
		void markerChanged(unsigned int marker, double value);
};


#endif
