// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDockWidget>
#include <QGridLayout>

#include <vector>

#include "hantekdso/controlspecification.h"
#include "hantekdso/enums.h"

class QLabel;
class QCheckBox;
class QComboBox;

class SiSpinBox;

struct DsoSettingsScope;
// struct ControlSpecification;

Q_DECLARE_METATYPE( std::vector< unsigned > )
Q_DECLARE_METATYPE( std::vector< double > )

/// \brief Dock window for the horizontal axis.
/// It contains the settings for the timebase and the display format.
class HorizontalDock : public QDockWidget {
    Q_OBJECT

  public:
    /// \brief Initializes the horizontal axis docking window.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    HorizontalDock( DsoSettingsScope *scope, const Dso::ControlSpecification *spec, QWidget *parent );

    /// \brief Changes the samplerate.
    /// \param samplerate The samplerate in seconds.
    double setSamplerate( double samplerate );
    /// \brief Changes the timebase.
    /// \param timebase The timebase in seconds.
    double setTimebase( double timebase );
    /// \brief Changes the record length if the new value is supported.
    /// \param recordLength The record length in samples.
    void setRecordLength( int recordLength );
    /// \brief Changes the format if the new value is supported.
    /// \param format The format for the horizontal axis.
    /// \return Index of format-value, -1 on error.
    int setFormat( Dso::GraphFormat format );
    /// \brief Updates the minimum and maximum of the samplerate spin box.
    /// \param minimum The minimum value the spin box should accept.
    /// \param maximum The minimum value the spin box should accept.
    void setSamplerateLimits( double minimum, double maximum );
    /// \brief Updates the mode and steps of the samplerate spin box.
    /// \param steps The steps value the spin box should accept.
    void setSamplerateSteps( int mode, QList< double > sampleSteps );
    void calculateSamplerateSteps( double timebase );
    /// \brief Changes the calibration frequency.
    /// \param calfreq The calibration frequency in hertz.
    double setCalfreq( double calfreq );

  public slots:
    /// \brief Loads settings into GUI
    /// \param scope Settings to load
    void loadSettings( DsoSettingsScope *scope );
    void triggerModeChanged( Dso::TriggerMode mode );

  protected:
    void closeEvent( QCloseEvent *event ) override;
    QGridLayout *dockLayout;        ///< The main layout for the dock window
    QWidget *dockWidget;            ///< The main widget for the dock window
    QLabel *samplerateLabel;        ///< The label for the samplerate spinbox
    QLabel *timebaseLabel;          ///< The label for the timebase spinbox
    QLabel *formatLabel;            ///< The label for the format combobox
    QLabel *calfreqLabel;           ///< The label for the calibration frequency spinbox
    SiSpinBox *samplerateSiSpinBox; ///< Selects the samplerate for acquisitions
    SiSpinBox *timebaseSiSpinBox;   ///< Selects the timebase for voltage graphs
    QComboBox *formatComboBox;      ///< Selects the way the sampled data is
                                    ///  interpreted and shown
    QComboBox *calfreqComboBox;     ///< Selects the calibration frequency

    DsoSettingsScope *scope;         ///< The settings provided by the parent class
    QList< double > timebaseSteps;   ///< Steps for the timebase spinbox
    QList< double > calfreqSteps;    ///< Steps for the calfreq spinbox
    QList< double > samplerateSteps; ///< Possible sampe rates

    QStringList formatStrings; ///< Strings for the formats

  protected slots:
    void samplerateSelected( double samplerate );
    void timebaseSelected( double timebase );
    void formatSelected( int index );
    void calfreqIndexSelected( int index );

  private:
    double samplerateRequest = 0;

  signals:
    void samplerateChanged( double samplerate );   ///< The samplerate has been changed
    void timebaseChanged( double timebase );       ///< The timebase has been changed
    void recordLengthChanged( int recordLength );  ///< The recordd length has been changed
    void formatChanged( Dso::GraphFormat format ); ///< The viewing format has been changed
    void calfreqChanged( double calfreq );         ///< The timebase has been changed
};
