// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>

#include "hantekdso/enums.h"

class SiSpinBox;
class DsoSettingsScope;
namespace Dso {
struct ControlSpecification;
}

/// \brief Dock window for the trigger settings.
/// It contains the settings for the trigger mode, source and slope.
class TriggerDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the trigger settings docking window.
    /// \param settings The target settings object.
    /// \param spec
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    TriggerDock(DsoSettingsScope *scope, const Dso::ControlSpecification* spec, QWidget *parent, Qt::WindowFlags flags = 0);

    /// \brief Changes the trigger mode if the new mode is supported.
    /// \param mode The trigger mode.
    void setMode(Dso::TriggerMode mode);

    /// \brief Changes the trigger source if the new source is supported.
    /// \param special true for a special channel (EXT, ...) as trigger source.
    /// \param id The number of the channel, that should be used as trigger.
    void setSource(bool special, unsigned int id);

    /// \brief Changes the trigger slope if the new slope is supported.
    /// \param slope The trigger slope.
    void setSlope(Dso::Slope slope);

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

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    const Dso::ControlSpecification* spec;

    QStringList sourceStandardStrings; ///< Strings for the standard trigger sources
    QStringList sourceSpecialStrings;  ///< Strings for the special trigger sources
  signals:
    void modeChanged(Dso::TriggerMode);                ///< The trigger mode has been changed
    void sourceChanged(bool special, unsigned int id); ///< The trigger source has been changed
    void slopeChanged(Dso::Slope);                     ///< The trigger slope has been changed
};
