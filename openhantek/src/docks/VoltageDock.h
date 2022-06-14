// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include "hantekdso/controlspecification.h"
#include "hantekdso/mathmodes.h"
#include "scopesettings.h"

#define ATTENUATION_MIN 1    ///< Minimum probe attenuation
#define ATTENUATION_MAX 1000 ///< Maximum probe attenuation

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
    VoltageDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent );

    /// \brief Sets the coupling for a channel.
    /// \param channel The channel, whose coupling should be set.
    /// \param couplingIndex The coupling-mode index.
    void setCoupling( ChannelID channel, unsigned couplingIndex );

    /// \brief Sets the gain for a channel.
    /// \param channel The channel, whose gain should be set.
    /// \param gain The gain in volts.
    void setGain( ChannelID channel, unsigned gainStepIndex );

    /// \brief Sets the probe attenuation for a channel.
    /// \param channel The channel, whose attn should be set.
    /// \param attn The attn value.
    void setAttn( ChannelID channel, double attnValue );

    /// \brief Sets the mode for the math channel.
    /// \param mathModeIndex The math-mode index.
    void setMode( unsigned mathModeIndex );

    /// \brief Enables/disables a channel.
    /// \param channel The channel, that should be enabled/disabled.
    /// \param used True if the channel should be enabled, false otherwise.
    void setUsed( ChannelID channel, bool used );

    /// \brief Set channel inverted.
    /// \param channel The channel, that should be inverted.
    /// \param used True if the channel should be inverted, false otherwise.
    void setInverted( ChannelID channel, bool inverted );

  public slots:
    /// \brief Loads settings into GUI
    /// \param scope Settings to load
    /// \param spec Current scope specifications
    void loadSettings( DsoSettingsScope *scope, const Dso::ControlSpecification *spec );

  protected:
    void closeEvent( QCloseEvent *event ) override;

    QGridLayout *dockLayout; ///< The main layout for the dock window
    QWidget *dockWidget;     ///< The main widget for the dock window

    struct ChannelBlock {
        QCheckBox *usedCheckBox;   ///< Enable/disable a specific channel
        QComboBox *gainComboBox;   ///< Select the vertical gain for the channels
        QComboBox *miscComboBox;   ///< Select coupling for real and mode for math channels
        QCheckBox *invertCheckBox; ///< Select if the channels should be displayed inverted
        QSpinBox *attnSpinBox;     ///< Enter the attenuation probe value
    };

    std::vector< ChannelBlock > channelBlocks;

    DsoSettingsScope *scope; ///< The settings provided by the parent class
    const Dso::ControlSpecification *spec;

    QStringList couplingStrings; ///< The strings for the couplings
    QStringList modeStrings;     ///< The strings for the math mode
    QStringList gainStrings;     ///< String representations for the gain steps
    QStringList attnStrings;     ///< String representations for the probe attn steps

  signals:
    void couplingChanged( ChannelID channel, Dso::Coupling coupling ); ///< A coupling has been selected
    void gainChanged( ChannelID channel, double gain );                ///< A gain has been selected
    void modeChanged( Dso::MathMode mode );                            ///< The mode for the math channels has been changed
    void usedChannelChanged( ChannelID channel, unsigned used );       ///< A channel has been enabled/disabled
    void probeAttnChanged( ChannelID channel, double probeAttn );      ///< A channel probe attenuation has been changed
    void invertedChanged( ChannelID channel, bool inverted );          ///< A channel "inverted" has been toggled
};
