////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file configpages.h
/// \brief Declares the pages for the DsoConfigDialog class.
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


#ifndef CONFIGPAGES_H
#define CONFIGPAGES_H


#include <QWidget>


#include "dsowidget.h"
#include "dso.h"


class ColorBox;
class DsoSettings;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QStringList;
class QLabel;


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigAnalysisPage                                   configpages.h
/// \brief Config page for the data analysis.
class DsoConfigAnalysisPage : public QWidget {
	Q_OBJECT
	
	public:
		DsoConfigAnalysisPage(DsoSettings *settings, QWidget *parent = 0);
		~DsoConfigAnalysisPage();
	
	public slots:
		void saveSettings();
	
	private:
		DsoSettings *settings;
		
		QVBoxLayout *mainLayout;
		
		QGroupBox *spectrumGroup;
		QGridLayout *spectrumLayout;
		QLabel *windowFunctionLabel;
		QComboBox *windowFunctionComboBox;
		
		QLabel *referenceLevelLabel;
		QDoubleSpinBox *referenceLevelSpinBox;
		QLabel *referenceLevelUnitLabel;
		QHBoxLayout *referenceLevelLayout;
		
		QLabel *minimumMagnitudeLabel;
		QDoubleSpinBox *minimumMagnitudeSpinBox;
		QLabel *minimumMagnitudeUnitLabel;
		QHBoxLayout *minimumMagnitudeLayout;
	
	private slots:
};


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigColorsPage                                     configpages.h
/// \brief Config page for the colors.
class DsoConfigColorsPage : public QWidget {
	Q_OBJECT
	
	public:
		DsoConfigColorsPage(DsoSettings *settings, QWidget *parent = 0);
		~DsoConfigColorsPage();
	
	public slots:
		void saveSettings();
	
	private:
		DsoSettings *settings;
		
		QVBoxLayout *mainLayout;
		
		QGroupBox *screenGroup;
		QGridLayout *screenLayout;
		QLabel *axesLabel, *backgroundLabel, *borderLabel, *gridLabel, *markersLabel, *textLabel;
		ColorBox *axesColorBox, *backgroundColorBox, *borderColorBox, *gridColorBox, *markersColorBox, *textColorBox;
		
		QGroupBox *graphGroup;
		QGridLayout *graphLayout;
		QLabel *channelLabel, *spectrumLabel;
		QList<QLabel *> colorLabel;
		QList<ColorBox *> channelColorBox;
		QList<ColorBox *> spectrumColorBox;
	
	private slots:
};


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigFilesPage                                      configpages.h
/// \brief Config page for file loading/saving.
class DsoConfigFilesPage : public QWidget {
	Q_OBJECT
	
	public:
		DsoConfigFilesPage(DsoSettings *settings, QWidget *parent = 0);
		~DsoConfigFilesPage();
	
	public slots:
		void saveSettings();
	
	private:
		DsoSettings *settings;
		
		QVBoxLayout *mainLayout;
		
		QGroupBox *configurationGroup;
		QVBoxLayout *configurationLayout;
		QCheckBox *saveOnExitCheckBox;
		QPushButton *saveNowButton;
		
		QGroupBox *exportGroup;
		QGridLayout *exportLayout;
		QLabel *imageWidthLabel;
		QSpinBox *imageWidthSpinBox;
		QLabel *imageHeightLabel;
		QSpinBox *imageHeightSpinBox;
	
	private slots:
};


////////////////////////////////////////////////////////////////////////////////
/// \class DsoConfigScopePage                                      configpages.h
/// \brief Config page for the scope screen.
class DsoConfigScopePage : public QWidget {
	Q_OBJECT
	
	public:
		DsoConfigScopePage(DsoSettings *settings, QWidget *parent = 0);
		~DsoConfigScopePage();
	
	public slots:
		void saveSettings();
	
	private:
		DsoSettings *settings;
		
		QVBoxLayout *mainLayout;
		
		QGroupBox *graphGroup;
		QGridLayout *graphLayout;
		QCheckBox *antialiasingCheckBox;
		QLabel *digitalPhosphorDepthLabel;
		QSpinBox *digitalPhosphorDepthSpinBox;
		QLabel *interpolationLabel;
		QComboBox *interpolationComboBox;
	
	private slots:
};


#endif
