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

/// \brief Dock window for the spectrum view.
/// It contains the magnitude for all channels and allows to enable/disable the
/// channels.
class SpectrumDock : public QDockWidget {
    Q_OBJECT

  public:
    SpectrumDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags = 0);

    int setMagnitude(int channel, double magnitude);
    int setUsed(int channel, bool used);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;              ///< The main layout for the dock window
    QWidget *dockWidget;                  ///< The main widget for the dock window
    QList<QCheckBox *> usedCheckBox;      ///< Enable/disable spectrum for a channel
    QList<QComboBox *> magnitudeComboBox; ///< Select the vertical magnitude for the spectrums

    DsoSettings *settings; ///< The settings provided by the parent class

    QList<double> magnitudeSteps; ///< The selectable magnitude steps
    QStringList magnitudeStrings; ///< String representations for the magnitude steps

  public slots:
    void magnitudeSelected(int index);
    void usedSwitched(bool checked);

  signals:
    void magnitudeChanged(unsigned int channel, double magnitude); ///< A magnitude has been selected
    void usedChanged(unsigned int channel, bool used);             ///< A spectrum has been enabled/disabled
};
