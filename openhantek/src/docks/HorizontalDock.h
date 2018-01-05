// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include <vector>

#include "hantekdso/enums.h"

class QLabel;
class QCheckBox;
class QComboBox;

class SiSpinBox;

struct DsoSettingsScope;

Q_DECLARE_METATYPE(std::vector<unsigned>)
Q_DECLARE_METATYPE(std::vector<double>)

/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the horizontal axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    HorizontalDock(DsoSettingsScope *scope, QWidget *parent, Qt::WindowFlags flags = 0);

    /// \brief Changes the frequencybase.
    /// \param frequencybase The frequencybase in hertz.
    void setFrequencybase(double timebase);
    /// \brief Changes the samplerate.
    /// \param samplerate The samplerate in seconds.
    void setSamplerate(double samplerate);
    /// \brief Changes the timebase.
    /// \param timebase The timebase in seconds.
    double setTimebase(double timebase);
    /// \brief Changes the record length if the new value is supported.
    /// \param recordLength The record length in samples.
    void setRecordLength(unsigned int recordLength);
    /// \brief Changes the format if the new value is supported.
    /// \param format The format for the horizontal axis.
    /// \return Index of format-value, -1 on error.
    int setFormat(Dso::GraphFormat format);
    /// \brief Updates the available record lengths in the combo box.
    /// \param recordLengths The available record lengths for the combo box.
    void setAvailableRecordLengths(const std::vector<unsigned> &recordLengths);
    /// \brief Updates the minimum and maximum of the samplerate spin box.
    /// \param minimum The minimum value the spin box should accept.
    /// \param maximum The minimum value the spin box should accept.
    void setSamplerateLimits(double minimum, double maximum);
    /// \brief Updates the mode and steps of the samplerate spin box.
    /// \param mode The mode value the spin box should accept.
    /// \param steps The steps value the spin box should accept.
    void setSamplerateSteps(int mode, QList<double> sampleSteps);

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

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    QList<double> timebaseSteps;     ///< Steps for the timebase spinbox

    QStringList formatStrings; ///< Strings for the formats

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
