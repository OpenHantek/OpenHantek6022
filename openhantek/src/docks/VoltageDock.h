// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include "scopesettings.h"
#include "hantekdso/controlspecification.h"
#include "post/postprocessingsettings.h"

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
    /// \param couplingIndex The coupling-mode index.
    void setCoupling(ChannelID channel, unsigned couplingIndex);

    /// \brief Sets the gain for a channel.
    /// \param channel The channel, whose gain should be set.
    /// \param gain The gain in volts.
    void setGain(ChannelID channel, unsigned gainStepIndex);

    /// \brief Sets the probe attenuation for a channel.
    /// \param channel The channel, whose attn should be set.
    /// \param attn The attn .
    void setAttn(ChannelID channel, bool attn);

    /// \brief Sets the mode for the math channel.
    /// \param mathModeIndex The math-mode index.
    void setMode(unsigned mathModeIndex);

    /// \brief Enables/disables a channel.
    /// \param channel The channel, that should be enabled/disabled.
    /// \param used True if the channel should be enabled, false otherwise.
    void setUsed(ChannelID channel, bool used);

    /// \brief Set channel inverted.
    /// \param channel The channel, that should be inverted.
    /// \param used True if the channel should be inverted, false otherwise.
    void setInverted(ChannelID channel, bool inverted);

  protected:
    void closeEvent(QCloseEvent *event);

    QGridLayout *dockLayout;           ///< The main layout for the dock window
    QWidget *dockWidget;               ///< The main widget for the dock window

    struct ChannelBlock {
        QCheckBox * usedCheckBox;   ///< Enable/disable a specific channel
        QComboBox * gainComboBox;   ///< Select the vertical gain for the channels
        QComboBox * miscComboBox;   ///< Select coupling for real and mode for math channels
        QCheckBox * invertCheckBox; ///< Select if the channels should be displayed inverted
        QCheckBox * attnCheckBox;   ///< Select if probe (x10) is used
    };

    std::vector<ChannelBlock> channelBlocks;

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    const Dso::ControlSpecification *spec;

    QStringList couplingStrings; ///< The strings for the couplings
    QStringList modeStrings;     ///< The strings for the math mode
    QStringList gainStrings;     ///< String representations for the gain steps
    QStringList attnStrings;     ///< String representations for the probe attn steps

  signals:
    void couplingChanged(ChannelID channel, Dso::Coupling coupling); ///< A coupling has been selected
    void gainChanged(ChannelID channel, double gain);                ///< A gain has been selected
    void modeChanged(Dso::MathMode mode);                            ///< The mode for the math channels has been changed
    void usedChanged(ChannelID channel, bool used);                  ///< A channel has been enabled/disabled
    void probeAttnChanged(ChannelID channel, bool probeUsed, double probeAttn); ///< A channel probe gain has been changed
    void invertedChanged(ChannelID channel, bool inverted);          ///< A channel "inverted" has been toggled
};
