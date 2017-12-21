// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include "definitions.h"
#include "settings.h"

class QLabel;
class QCheckBox;
class QComboBox;

class SiSpinBox;

////////////////////////////////////////////////////////////////////////////////
/// \class HorizontalDock                                          dockwindows.h
/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
    Q_OBJECT

  public:
    HorizontalDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags = 0);

    void setFrequencybase(double timebase);
    void setSamplerate(double samplerate);
    double setTimebase(double timebase);
    void setRecordLength(unsigned int recordLength);
    int setFormat(Dso::GraphFormat format);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;           ///< The main layout for the dock window
    QWidget *dockWidget;               ///< The main widget for the dock window
    QLabel *samplerateLabel;           ///< The label for the samplerate spinbox
    QLabel *timebaseLabel;             ///< The label for the timebase spinbox
    QLabel *frequencybaseLabel;        ///< The label for the frequencybase spinbox
    QLabel *recordLengthLabel;         ///< The label for the record length combobox
    QLabel *formatLabel;               ///< The label for the format combobox
    SiSpinBox *samplerateSiSpinBox;    ///< Selects the samplerate for aquisitions
    SiSpinBox *timebaseSiSpinBox;      ///< Selects the timebase for voltage graphs
    SiSpinBox *frequencybaseSiSpinBox; ///< Selects the frequencybase for spectrum graphs
    QComboBox *recordLengthComboBox;   ///< Selects the record length for aquisitions
    QComboBox *formatComboBox;         ///< Selects the way the sampled data is
                                       /// interpreted and shown

    DsoSettings *settings = nullptr; ///< The settings provided by the parent class
    QList<double> timebaseSteps;     ///< Steps for the timebase spinbox

    QStringList formatStrings; ///< Strings for the formats

  public slots:
    void availableRecordLengthsChanged(const std::vector<unsigned> &recordLengths);
    void samplerateLimitsChanged(double minimum, double maximum);
    void samplerateSet(int mode, QList<double> sampleSteps);

  protected slots:
    void frequencybaseSelected(double frequencybase);
    void samplerateSelected(double samplerate);
    void timebaseSelected(double timebase);
    void recordLengthSelected(int index);
    void formatSelected(int index);

  signals:
    void frequencybaseChanged(double frequencybase);      ///< The frequencybase has been changed
    void samplerateChanged(double samplerate);            ///< The samplerate has been changed
    void timebaseChanged(double timebase);                ///< The timebase has been changed
    void recordLengthChanged(unsigned long recordLength); ///< The recordd length has been changed
    void formatChanged(Dso::GraphFormat format);          ///< The viewing format has been changed
};
