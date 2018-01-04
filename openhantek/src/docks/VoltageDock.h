// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QDockWidget>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

#include "settings.h"

class SiSpinBox;

/// \brief Dock window for the voltage channel settings.
/// It contains the settings for gain and coupling for both channels and
/// allows to enable/disable the channels.
class VoltageDock : public QDockWidget {
    Q_OBJECT

  public:
    VoltageDock(DsoSettings *settings, QWidget *parent, Qt::WindowFlags flags = 0);

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

    DsoSettings *settings; ///< The settings provided by the parent class

    QStringList couplingStrings; ///< The strings for the couplings
    QStringList modeStrings;     ///< The strings for the math mode
    QStringList gainStrings;     ///< String representations for the gain steps

  signals:
    void couplingChanged(unsigned int channel, Dso::Coupling coupling); ///< A coupling has been selected
    void gainChanged(unsigned int channel, double gain);                ///< A gain has been selected
    void modeChanged(Dso::MathMode mode);              ///< The mode for the math channels has been changed
    void usedChanged(unsigned int channel, bool used); ///< A channel has been enabled/disabled
};
