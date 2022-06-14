// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include "scopesettings.h"

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
    /// \brief Initializes the spectrum view docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    SpectrumDock( DsoSettingsScope *scope, QWidget *parent );

    /// \brief Sets the magnitude for a channel.
    /// \param channel The channel, whose magnitude should be set.
    /// \param magnitude The magnitude in dB.
    /// \return Index of magnitude-value, -1 on error.
    int setMagnitude( ChannelID channel, double magnitude );

    /// \brief Enables/disables a channel.
    /// \param channel The channel, that should be enabled/disabled.
    /// \param used True if the channel should be enabled, false otherwise.
    /// \return Index of channel, INT_MAX on error.
    unsigned setUsed( ChannelID channel, bool used );

    /// \brief Enables/disables all channels.
    /// \param enabled True if the channel should be enabled, false otherwise.
    void enableSpectrumDock( bool enabled );

    /// \brief Changes the frequencybase.
    /// \param frequencybase The frequencybase in hertz.
    void setFrequencybase( double timebase );

  public slots:
    /// \brief Loads settings into GUI
    /// \param scope Settings to load
    void loadSettings( DsoSettingsScope *scope );

    /// \brief Called when the samplerate from horizontal dock changes its value.
    /// \param samplerare The samplerate in hertz.
    void setSamplerate( double samplerate );

  private slots:
    void frequencybaseSelected( double frequencybase );

  protected:
    void closeEvent( QCloseEvent *event ) override;

    QGridLayout *dockLayout; ///< The main layout for the dock window
    QWidget *dockWidget;     ///< The main widget for the dock window

    struct ChannelBlock {
        QCheckBox *usedCheckBox;      ///< Enable/disable a specific channel
        QComboBox *magnitudeComboBox; ///< Select the vertical magnitude for the spectrums
    };

    std::vector< ChannelBlock > channelBlocks;

    DsoSettingsScope *scope; ///< The settings provided by the parent class

    std::vector< double > magnitudeSteps; ///< The selectable magnitude steps in dB/div
    QStringList magnitudeStrings;         ///< String representations for the magnitude steps
    QLabel *frequencybaseLabel;           ///< The label for the frequencybase spinbox
    SiSpinBox *frequencybaseSiSpinBox;    ///< Selects the frequencybase for spectrum graphs

  signals:
    void magnitudeChanged( ChannelID channel, double magnitude ); ///< A magnitude has been selected
    void usedChannelChanged( ChannelID channel, unsigned used );  ///< A spectrum has been enabled/disabled
    void frequencybaseChanged( double frequencybase );            ///< The frequencybase has been changed
};
