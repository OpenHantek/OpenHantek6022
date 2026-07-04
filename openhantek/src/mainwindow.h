// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "post/ppresult.h"
#include <QElapsedTimer>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <functional>
#include <memory>

#include "scopesettings.h"
#include "usb/devicereconnectionsupervisor.h"

class SpectrumGenerator;
class HantekDsoControl;
class DsoSettings;
class ExporterRegistry;
class DsoWidget;
class HorizontalDock;
class TriggerDock;
class SpectrumDock;
class VoltageDock;
class QAction;
class QLabel;
class QPlainTextEdit;
class RemoteServer;

namespace Ui {
class MainWindow;
}

/// \brief The main window of the application.
/// The main window contains the classic oszilloscope-screen and the gui
/// elements used to control the oszilloscope.
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow( HantekDsoControl *dsoControl, DsoSettings *dsoSettings, ExporterRegistry *exporterRegistry,
                         QWidget *parent = nullptr );
    ~MainWindow() override;
    QElapsedTimer elapsedTime;

  public slots:
    void showNewData( std::shared_ptr< PPresult > newData );
    void exporterStatusChanged( const QString &exporterName, const QString &status );
    void exporterProgressChanged();
    void deviceConnectionStateChanged( DeviceConnectionState state, const QString &message );

  protected:
    void closeEvent( QCloseEvent *event ) override;

  private:
    Ui::MainWindow *ui;
    QIcon iconPause;
    QIcon iconPlay;
    QLineEdit *commandEdit = nullptr;
    QString lastSaveAsDir = "";

    // Central widgets
    DsoWidget *dsoWidget;

    // Docking windows
    VoltageDock *voltageDock = nullptr;
    HorizontalDock *horizontalDock = nullptr;
    TriggerDock *triggerDock = nullptr;
    SpectrumDock *spectrumDock = nullptr;

    // DSO control layer (lives in its own thread)
    HantekDsoControl *dsoControl = nullptr;
    const Dso::ControlSpecification *mSpec = nullptr;

    // Settings used for the whole program
    DsoSettings *dsoSettings;
    ExporterRegistry *exporterRegistry;

    // Last processed acquisition, used for Autoset and remote measurements
    std::shared_ptr< PPresult > lastResult;

    // Remote control server (menu controlled, or started with --server PORT)
    RemoteServer *remoteServer = nullptr;
    QAction *actionRemoteServer = nullptr;
    QLabel *remoteStatusLabel = nullptr;
    QPlainTextEdit *remoteLogView = nullptr;
    QStringList remoteLog;
    quint16 remotePort = 5025;
    void setRemoteServerEnabled( bool enabled );
    void updateRemoteStatusLabel();
    void appendRemoteLog( const QString &line );
    void showRemoteLogDialog();

    void autoSet();
    void autoSetRun();
    int autoSetRetries = 0;
    void startSingleShot();
    void applyFontSize( int fontSize );
    void reloadSettings();
    bool saveScreenshot( const QString &fileName );
    QString executeRemoteCommand( const QString &line );
    QString executeRemoteSetOrQuery( const QString &cmd, const QString &arg, DsoSettingsScope *scope,
                                     const std::function< bool( const QString &, bool * ) > &onOff );
    QString executeRemoteHorTrigOrQuery( const QString &cmd, const QString &arg, DsoSettingsScope *scope );

    // Taking screenshots
    enum screenshotType_t { SCREENSHOT, HARDCOPY, PRINTER };
    screenshotType_t screenshotType;
    void screenShot( screenshotType_t screenshotType = SCREENSHOT, bool autoSave = false );

    bool openDocument( QString docName );
    void setDeviceCommandUiEnabled( bool enabled );

    QList< QWidget * > deviceCommandWidgets;
    QList< QAction * > deviceCommandActions;

  signals:
    void settingsLoaded( DsoSettingsScope *scope, const Dso::ControlSpecification *spec );
};
