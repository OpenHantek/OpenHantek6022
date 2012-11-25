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
		
		QGridLayout *mainLayout; ///< The main layout for this widget
		GlGenerator *generator; ///< The generator for the OpenGL vertex arrays
		GlScope *mainScope; ///< The main scope screen
		GlScope *zoomScope; ///< The optional magnified scope screen
		LevelSlider *offsetSlider; ///< The sliders for the graph offsets
		LevelSlider *triggerPositionSlider; ///< The slider for the pretrigger
		LevelSlider *triggerLevelSlider; ///< The sliders for the trigger level
		LevelSlider *markerSlider; ///< The sliders for the markers
		
		QHBoxLayout *settingsLayout; ///< The table for the settings info
		QLabel *settingsTriggerLabel; ///< The trigger details
		QLabel *settingsRecordLengthLabel; ///< The record length
		QLabel *settingsSamplerateLabel; ///< The samplerate
		QLabel *settingsTimebaseLabel; ///< The timebase of the main scope
		QLabel *settingsFrequencybaseLabel; ///< The frequencybase of the main scope
		
		QHBoxLayout *markerLayout; ///< The table for the marker details
		QLabel *markerInfoLabel; ///< The info about the zoom factor
		QLabel *markerTimeLabel; ///< The time period between the markers
		QLabel *markerFrequencyLabel; ///< The frequency for the time period
		QLabel *markerTimebaseLabel; ///< The timebase for the zoomed scope
		QLabel *markerFrequencybaseLabel; ///< The frequencybase for the zoomed scope
		
		QGridLayout *measurementLayout; ///< The table for the signal details
		QList<QLabel *> measurementNameLabel; ///< The name of the channel
		QList<QLabel *> measurementGainLabel; ///< The gain for the voltage (V/div)
		QList<QLabel *> measurementMagnitudeLabel; ///< The magnitude for the spectrum (dB/div)
		QList<QLabel *> measurementMiscLabel; ///< Coupling or math mode
		QList<QLabel *> measurementAmplitudeLabel; ///< Amplitude of the signal (V)
		QList<QLabel *> measurementFrequencyLabel; ///< Frequency of the signal (Hz)
		
		DsoSettings *settings; ///< The settings provided by the main window
		
		DataAnalyzer *dataAnalyzer; ///< The data source provided by the main window
	
	public slots:
		// Horizontal axis
		//void horizontalFormatChanged(HorizontalFormat format);
		void updateFrequencybase(double frequencybase);
		void updateSamplerate(double samplerate);
		void updateTimebase(double timebase);
		
		// Trigger
		void updateTriggerMode();
		void updateTriggerSlope();
		void updateTriggerSource();
		
		// Spectrum
		void updateSpectrumMagnitude(unsigned int channel);
		void updateSpectrumUsed(unsigned int channel, bool used);
		
		// Vertical axis
		void updateVoltageCoupling(unsigned int channel);
		void updateMathMode();
		void updateVoltageGain(unsigned int channel);
		void updateVoltageUsed(unsigned int channel, bool used);
		
		// Menus
		void updateRecordLength(unsigned long size);
		
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
		void offsetChanged(unsigned int channel, double value); ///< A graph offset has been changed
		void triggerPositionChanged(double value); ///< The pretrigger has been changed
		void triggerLevelChanged(unsigned int channel, double value); ///< A trigger level has been changed
		void markerChanged(unsigned int marker, double value); ///< A marker position has been changed
};


#endif
