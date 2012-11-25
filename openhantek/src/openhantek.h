////////////////////////////////////////////////////////////////////////////////
//
//  OpenHantek
/// \file openhantek.h
/// \brief Declares the HantekDsoMainWindow class.
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


#ifndef HANTEKDSO_H
#define HANTEKDSO_H


#include <QMainWindow>


class QActionGroup;
class QLineEdit;

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
		
		// Device management
		void connectSignals();
		void initializeDevice();

		// Settings
		int readSettings(const QString &fileName = QString());
		int writeSettings(const QString &fileName = QString());
		
		// Window translation events
		void moveEvent(QMoveEvent * event);
		void resizeEvent(QResizeEvent * event);

		// Actions
		QAction *newAction, *openAction, *saveAction, *saveAsAction;
		QAction *printAction, *exportAsAction;
		QAction *exitAction;
		
		QAction *configAction;
		QAction *startStopAction;
		QAction *digitalPhosphorAction, *zoomAction;
		
		QAction *aboutAction, *aboutQtAction;
		
#ifdef DEBUG
		QAction *commandAction;
#endif

		// Menus
		QMenu *fileMenu;
		QMenu *viewMenu, *dockMenu, *toolbarMenu;
		QMenu *oscilloscopeMenu;
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
		
		// Other widgets
#ifdef DEBUG
		QLineEdit *commandEdit;
#endif
		
		// Data handling classes
		DataAnalyzer *dataAnalyzer;
		DsoControl *dsoControl;
		
		// Other variables
		QString currentFile;
		
		// Settings used for the whole program
		DsoSettings *settings;
		
	private slots:
		// File operations
		int open();
		int save();
		int saveAs();
		// View
		void digitalPhosphor(bool enabled);
		void zoom(bool enabled);
		// Oscilloscope control
		void started();
		void stopped();
		// Other
		void config();
		void about();
		
		// Settings management
		void applySettings();
		void updateSettings();
		
		void recordTimeChanged(double duration);
		void samplerateChanged(double samplerate);
		void recordLengthSelected(unsigned long recordLength);
		void samplerateSelected();
		void timebaseSelected();
		void updateOffset(unsigned int channel);
		void updateUsed(unsigned int channel);
		void updateVoltageGain(unsigned int channel);
		
#ifdef DEBUG
		void sendCommand();
#endif
	
	signals:
		void settingsChanged(); ///< The settings have changed (Option dialog, loading...)
};


#endif
