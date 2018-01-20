#pragma once
#include "post/ppresult.h"
#include <QMainWindow>
#include <memory>

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
    explicit MainWindow(HantekDsoControl *dsoControl, DsoSettings *mSettings, ExporterRegistry *exporterRegistry,
                        QWidget *parent = 0);
    ~MainWindow();
  public slots:
    void showNewData(std::shared_ptr<PPresult> data);
    void exporterStatusChanged(const QString &exporterName, const QString &status);
    void exporterProgressChanged();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    Ui::MainWindow *ui;

    // Central widgets
    DsoWidget *dsoWidget;

    // Settings used for the whole program
    DsoSettings *mSettings;
    ExporterRegistry *exporterRegistry;
};
