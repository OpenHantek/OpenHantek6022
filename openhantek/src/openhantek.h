////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file openhantek.h
/// \brief Declares the HantekDsoMainWindow class.
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


#ifndef HANTEKDSO_H
#define HANTEKDSO_H


#include <QMainWindow>


class QActionGroup;

class DataAnalyzer;
class DsoControl;
class DsoSettings;
class DsoWidget;
class HorizontalDock;
class TriggerDock;
class SpectrumDock;
class VoltageDock;


////////////////////////////////////////////////////////////////////////////////
/// \class OpenHantekMainWindow                                     openhantek.h
/// \brief The main window of the application.
/// The main window contains the classic oszilloscope-screen and the gui
/// elements used to control the oszilloscope.
class OpenHantekMainWindow : public QMainWindow {
	Q_OBJECT

	public:
		OpenHantekMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
		~OpenHantekMainWindow();

	protected:
		void closeEvent(QCloseEvent *event);

	private:
		// GUI creation
		void createActions();
		void createMenus();
		void createToolBars();
		void createStatusBar();
		void createDockWindows();

		// Settings
		void readSettings(const QString &fileName = QString());
		void writeSettings(const QString &fileName = QString());

		// Actions
		QAction *newAction, *openAction, *saveAction, *saveAsAction;
		QAction *printAction, *exportAsAction;
		QAction *exitAction;
		
		QAction *configAction;
		QAction *startStopAction;
		QActionGroup *bufferSizeActionGroup;
		QAction *bufferSizeSmallAction, *bufferSizeLargeAction;
		QAction *digitalPhosphorAction, *zoomAction;
		
		QAction *aboutAction, *aboutQtAction;

		// Menus
		QMenu *fileMenu;
		QMenu *viewMenu;
		QMenu *oscilloscopeMenu, *bufferSizeMenu;
		QMenu *helpMenu;

		// Toolbars
		QToolBar *fileToolBar, *oscilloscopeToolBar, *viewToolBar;
		
		// Docking windows
		HorizontalDock *horizontalDock;
		TriggerDock *triggerDock;
		SpectrumDock *spectrumDock;
		VoltageDock *voltageDock;
		
		// Central widgets
		DsoWidget *dsoWidget;
		
		// Data handling classes
		DataAnalyzer *dataAnalyzer;
		DsoControl *dsoControl;
		
		// Other variables
		QString currentFile;
		
		DsoSettings *settings;
		
	private slots:
		// File operations
		void open();
		bool save();
		bool saveAs();
		// View
		void digitalPhosphor(bool enabled);
		void zoom(bool enabled);
		// Oscilloscope control
		void started();
		void stopped();
		// Other
		void config();
		void about();
		
		void applySettings();
		
		void bufferSizeSelected(QAction *action);
		void updateOffset(unsigned int channel);
		void updateTimebase();
		void updateUsed(unsigned int channel);
		void updateVoltageGain(unsigned int channel);
	
	signals:
		void settingsChanged(); ///< The settings have changed (Option dialog, loading...)
};


#endif
