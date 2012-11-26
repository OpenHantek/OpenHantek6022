////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file settings.h
/// \brief Declares the DsoSettings class.
//
//  Copyright (C) 2010, 2011  Oliver Haag
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


#ifndef SETTINGS_H
#define SETTINGS_H


#include <QColor>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QString>


#include "dso.h"


////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptionsWindowPanel                             settings.h
/// \brief Holds the position and state of a docking window or toolbar.
struct DsoSettingsOptionsWindowPanel {
	bool floating; ///< true, if the panel is floating
	QPoint position; ///< Position of the panel
	bool visible; ///< true, if the panel is shown
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptionsWindowDock                              settings.h
/// \brief Holds the layout of the docking windows.
struct DsoSettingsOptionsWindowDock {
	DsoSettingsOptionsWindowPanel horizontal; ///< "Horizontal" docking window
	DsoSettingsOptionsWindowPanel spectrum; ///< "Spectrum" docking window
	DsoSettingsOptionsWindowPanel trigger; ///< "Trigger" docking window
	DsoSettingsOptionsWindowPanel voltage; ///< "Voltage" docking window
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptionsWindowToolbar                           settings.h
/// \brief Holds the layout of the toolbars.
struct DsoSettingsOptionsWindowToolbar {
	DsoSettingsOptionsWindowPanel file; ///< "File" toolbar
	DsoSettingsOptionsWindowPanel oscilloscope; ///< "Oscilloscope" toolbar
	DsoSettingsOptionsWindowPanel view; ///< The "View" toolbar
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptionsWindow                                  settings.h
/// \brief Holds the layout of the main window.
struct DsoSettingsOptionsWindow {
	QPoint position; ///< Position of the main window
	QSize size; ///< Size of the main window
	DsoSettingsOptionsWindowDock dock; ///< Docking windows
	DsoSettingsOptionsWindowToolbar toolbar; ///< Toolbars
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptions                                        settings.h
/// \brief Holds the general options of the program.
struct DsoSettingsOptions {
	bool alwaysSave; ///< Always save the settings on exit
	QSize imageSize; ///< Size of exported images in pixels
	DsoSettingsOptionsWindow window; ///< Window layout
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeHorizontal                                settings.h
/// \brief Holds the settings for the horizontal axis.
struct DsoSettingsScopeHorizontal {
	Dso::GraphFormat format; ///< Graph drawing mode of the scope
	double frequencybase; ///< Frequencybase in Hz/div
	double marker[2]; ///< Marker positions in div
	double timebase; ///< Timebase in s/div
	unsigned int recordLength; ///< Sample count
	double samplerate; ///< The samplerate of the oscilloscope in S
	bool samplerateSet; ///< The samplerate was set by the user, not the timebase
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeTrigger                                   settings.h
/// \brief Holds the settings for the trigger.
struct DsoSettingsScopeTrigger {
	bool filter; ///< Not sure what this is good for...
	Dso::TriggerMode mode; ///< Automatic, normal or single trigger
	double position; ///< Horizontal position for pretrigger
	Dso::Slope slope; ///< Rising or falling edge causes trigger
	unsigned int source; ///< Channel that is used as trigger source
	bool special; ///< true if the trigger source is not a standard channel
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeSpectrum                                  settings.h
/// \brief Holds the settings for the spectrum analysis.
struct DsoSettingsScopeSpectrum {
	double magnitude; ///< The vertical resolution in dB/div
	QString name; ///< Name of this channel
	double offset; ///< Vertical offset in divs
	bool used; ///< true if the spectrum is turned on
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScopeVoltage                                   settings.h
/// \brief Holds the settings for the normal voltage graphs.
struct DsoSettingsScopeVoltage {
	double gain; ///< The vertical resolution in V/div
	int misc; ///< Different enums, coupling for real- and mode for math-channels
	QString name; ///< Name of this channel
	double offset; ///< Vertical offset in divs
	double trigger; ///< Trigger level in V
	bool used; ///< true if this channel is enabled
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsScope                                          settings.h
/// \brief Holds the settings for the oscilloscope.
struct DsoSettingsScope {
	DsoSettingsScopeHorizontal horizontal; ///< Settings for the horizontal axis
	DsoSettingsScopeTrigger trigger; ///< Settings for the trigger
	QList<DsoSettingsScopeSpectrum> spectrum; ///< Spectrum analysis settings
	QList<DsoSettingsScopeVoltage> voltage; ///< Settings for the normal graphs
	
	unsigned int physicalChannels; ///< Number of real channels (No math etc.)
	Dso::WindowFunction spectrumWindow; ///< Window function for DFT
	double spectrumReference; ///< Reference level for spectrum in dBm
	double spectrumLimit; ///< Minimum magnitude of the spectrum (Avoids peaks)
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsColorValues                                    settings.h
/// \brief Holds the color values for the oscilloscope screen.
struct DsoSettingsColorValues {
	QColor axes; ///< X- and Y-axis and subdiv lines on them
	QColor background; ///< The scope background
	QColor border; ///< The border of the scope screen
	QColor grid; ///< The color of the grid
	QColor markers; ///< The color of the markers
	QList<QColor> spectrum; ///< The colors of the spectrum graphs
	QColor text; ///< The default text color
	QList<QColor> voltage; ///< The colors of the voltage graphs
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsViewColor                                      settings.h
/// \brief Holds the settings for the used colors on the screen and on paper.
struct DsoSettingsViewColor {
	DsoSettingsColorValues screen; ///< Colors for the screen
	DsoSettingsColorValues print; ///< Colors for printout
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsView                                           settings.h
/// \brief Holds all view settings.
struct DsoSettingsView {
	DsoSettingsViewColor color; ///< Used colors
	bool antialiasing; ///< Antialiasing for the graphs
	bool digitalPhosphor; ///< true slowly fades out the previous graphs
	int digitalPhosphorDepth; ///< Number of channels shown at one time
	Dso::InterpolationMode interpolation; ///< Interpolation mode for the graph
	bool screenColorImages; ///< true exports images with screen colors
	bool zoom; ///< true if the magnified scope is enabled
};

////////////////////////////////////////////////////////////////////////////////
/// \class DsoSettings                                                settings.h
/// \brief Holds the settings of the program.
class DsoSettings : public QObject {
	Q_OBJECT
	
	public:
		DsoSettings(QWidget *parent = 0);
		~DsoSettings();
		
		void setChannelCount(unsigned int channels);
		
		DsoSettingsOptions options; ///< General options of the program
		DsoSettingsScope scope; ///< All oscilloscope related settings
		DsoSettingsView view; ///< All view related settings
	
	public slots:
		// Saving to and loading from configuration files
		int load(const QString &fileName = QString());
		int save(const QString &fileName = QString());
};


#endif
