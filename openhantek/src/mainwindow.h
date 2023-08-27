// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#include "post/ppresult.h"
#include <QElapsedTimer>
#include <QLineEdit>
#include <QMainWindow>
#include <memory>

#include "scopesettings.h"

class SpectrumGenerator;
class HantekDsoControl;
class DsoSettings;
class ExporterRegistry;
class DsoWidget;
class HorizontalDock;
class TriggerDock;
class SpectrumDock;
class VoltageDock;

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

  protected:
    void closeEvent( QCloseEvent *event ) override;

  private:
    Ui::MainWindow *ui;
    QIcon iconPause;
    QIcon iconPlay;
    QLineEdit *commandEdit;

    // Central widgets
    DsoWidget *dsoWidget;

    // Settings used for the whole program
    DsoSettings *dsoSettings;
    ExporterRegistry *exporterRegistry;

    // Taking screenshots
    enum screenshotType_t { SCREENSHOT, HARDCOPY, PRINTER };
    screenshotType_t screenshotType;
    void screenShot( screenshotType_t screenshotType = SCREENSHOT, bool autoSave = false );

    bool openDocument( QString docName );

  signals:
    void settingsLoaded( DsoSettingsScope *scope, const Dso::ControlSpecification *spec );
};
