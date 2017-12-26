// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include "settings.h"

class QLabel;
class QCheckBox;
class QComboBox;

class SiSpinBox;

/// \brief Dock window for the trigger settings.
/// It contains the settings for the trigger mode, source and slope.
class TriggerDock : public QDockWidget {
    Q_OBJECT

  public:
    TriggerDock(DsoSettings *settings, const std::vector<std::string>& specialTriggers, QWidget *parent, Qt::WindowFlags flags = 0);

    int setMode(Dso::TriggerMode mode);
    int setSource(bool special, unsigned int id);
    int setSlope(Dso::Slope slope);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;   ///< The main layout for the dock window
    QWidget *dockWidget;       ///< The main widget for the dock window
    QLabel *modeLabel;         ///< The label for the trigger mode combobox
    QLabel *sourceLabel;       ///< The label for the trigger source combobox
    QLabel *slopeLabel;        ///< The label for the trigger slope combobox
    QComboBox *modeComboBox;   ///< Select the triggering mode
    QComboBox *sourceComboBox; ///< Select the source for triggering
    QComboBox *slopeComboBox;  ///< Select the slope that causes triggering

    DsoSettings *settings; ///< The settings provided by the parent class

    QStringList modeStrings;           ///< Strings for the trigger modes
    QStringList sourceStandardStrings; ///< Strings for the standard trigger sources
    QStringList sourceSpecialStrings;  ///< Strings for the special trigger sources
    QStringList slopeStrings;          ///< Strings for the trigger slopes

  protected slots:
    void modeSelected(int index);
    void slopeSelected(int index);
    void sourceSelected(int index);

  signals:
    void modeChanged(Dso::TriggerMode);                ///< The trigger mode has been changed
    void sourceChanged(bool special, unsigned int id); ///< The trigger source has been changed
    void slopeChanged(Dso::Slope);                     ///< The trigger slope has been changed
};
