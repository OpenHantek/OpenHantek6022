// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMainWindow>
#include <QTimer>
#include <memory>

class QActionGroup;
class QLineEdit;

class DataAnalyzer;
class HantekDsoControl;
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
  OpenHantekMainWindow(std::shared_ptr<HantekDsoControl> dsoControl, std::shared_ptr<DataAnalyzer> dataAnalyser);

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
  void applySettingsToDevice();

  // Settings
  int readSettings(const QString &fileName = QString());
  int writeSettings(const QString &fileName = QString());

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
  std::shared_ptr<HantekDsoControl> dsoControl;
  std::shared_ptr<DataAnalyzer> dataAnalyzer;

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
  void
  settingsChanged(); ///< The settings have changed (Option dialog, loading...)
};
