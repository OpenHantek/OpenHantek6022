// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include "scopesettings.h"
#include "hantekdso/controlspecification.h"

class SiSpinBox;

/// \brief Dock window for the voltage channel settings.
/// It contains the settings for gain and coupling for both channels and
/// allows to enable/disable the channels.
class VoltageDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the vertical axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    VoltageDock(DsoSettingsScope *scope, const Dso::ControlSpecification* spec, QWidget *parent, Qt::WindowFlags flags = 0);

    /// \brief Sets the coupling for a channel.
    /// \param channel The channel, whose coupling should be set.
    /// \param coupling The coupling-mode.
    void setCoupling(ChannelID channel, unsigned couplingIndex);

    /// \brief Sets the gain for a channel.
    /// \param channel The channel, whose gain should be set.
    /// \param gain The gain in volts.
    void setGain(ChannelID channel, unsigned gainStepIndex);

    /// \brief Sets the mode for the math channel.
    /// \param mode The math-mode.
    void setMode(Dso::MathMode mode);

    /// \brief Enables/disables a channel.
    /// \param channel The channel, that should be enabled/disabled.
    /// \param used True if the channel should be enabled, false otherwise.
    void setUsed(ChannelID channel, bool used);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;           ///< The main layout for the dock window
    QWidget *dockWidget;               ///< The main widget for the dock window

    struct ChannelBlock {
        QCheckBox * usedCheckBox;   ///< Enable/disable a specific channel
        QComboBox * gainComboBox;   ///< Select the vertical gain for the channels
        QComboBox * miscComboBox;   ///< Select coupling for real and mode for math channels
        QCheckBox * invertCheckBox; ///< Select if the channels should be displayed inverted
    };

    std::vector<ChannelBlock> channelBlocks;

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    const Dso::ControlSpecification *spec;

    QStringList couplingStrings; ///< The strings for the couplings
    QStringList modeStrings;     ///< The strings for the math mode
    QStringList gainStrings;     ///< String representations for the gain steps

  signals:
    void couplingChanged(ChannelID channel, Dso::Coupling coupling); ///< A coupling has been selected
    void gainChanged(ChannelID channel, double gain);                ///< A gain has been selected
    void modeChanged(Dso::MathMode mode);              ///< The mode for the math channels has been changed
    void usedChanged(ChannelID channel, bool used); ///< A channel has been enabled/disabled
};
