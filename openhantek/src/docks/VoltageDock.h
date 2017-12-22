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

/// \brief Dock window for the voltage channel settings.
/// It contains the settings for gain and coupling for both channels and
/// allows to enable/disable the channels.
class VoltageDock : public QDockWidget {
    Q_OBJECT

  public:
    VoltageDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags = 0);

    int setCoupling(int channel, Dso::Coupling coupling);
    int setGain(int channel, double gain);
    int setMode(Dso::MathMode mode);
    int setUsed(int channel, bool used);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;           ///< The main layout for the dock window
    QWidget *dockWidget;               ///< The main widget for the dock window
    QList<QCheckBox *> usedCheckBox;   ///< Enable/disable a specific channel
    QList<QComboBox *> gainComboBox;   ///< Select the vertical gain for the channels
    QList<QComboBox *> miscComboBox;   ///< Select coupling for real and mode for math channels
    QList<QCheckBox *> invertCheckBox; ///< Select if the channels should be displayed inverted

    DsoSettings *settings; ///< The settings provided by the parent class

    QStringList couplingStrings; ///< The strings for the couplings
    QStringList modeStrings;     ///< The strings for the math mode
    QList<double> gainSteps;     ///< The selectable gain steps
    QStringList gainStrings;     ///< String representations for the gain steps

  protected slots:
    void gainSelected(int index);
    void miscSelected(int index);
    void usedSwitched(bool checked);
    void invertSwitched(bool checked);

  signals:
    void couplingChanged(unsigned int channel, Dso::Coupling coupling); ///< A coupling has been selected
    void gainChanged(unsigned int channel, double gain);                ///< A gain has been selected
    void modeChanged(Dso::MathMode mode);              ///< The mode for the math channels has been changed
    void usedChanged(unsigned int channel, bool used); ///< A channel has been enabled/disabled
};
