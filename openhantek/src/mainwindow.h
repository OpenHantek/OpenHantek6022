#pragma once
#include <QMainWindow>

class DataAnalyzer;
class HantekDsoControl;
class DsoSettings;
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
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(HantekDsoControl *dsoControl, DataAnalyzer *dataAnalyser, DsoSettings *settings, QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
private:
    Ui::MainWindow *ui;

    // Docking windows
    HorizontalDock *horizontalDock;
    TriggerDock *triggerDock;
    SpectrumDock *spectrumDock;
    VoltageDock *voltageDock;

    // Central widgets
    DsoWidget *dsoWidget;

    // Data handling classes
    HantekDsoControl *dsoControl;
    DataAnalyzer *dataAnalyzer;

    // Settings used for the whole program
    DsoSettings *settings;
};
