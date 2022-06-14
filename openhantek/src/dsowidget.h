// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <memory>

#include "glscope.h"
#include "hantekdso/controlspecification.h"
#include "levelslider.h"
#include "viewsettings.h"

class SpectrumGenerator;
struct DsoSettingsScope;
struct DsoSettingsView;
class DataGrid;

/// \brief The widget for the oszilloscope-screen
/// This widget contains the scopes and all level sliders.
class DsoWidget : public QWidget {
    Q_OBJECT

  public:
    struct Sliders {
        LevelSlider *voltageOffsetSlider;   ///< The sliders for the graph offsets
        LevelSlider *triggerPositionSlider; ///< The slider for the pretrigger
        LevelSlider *triggerLevelSlider;    ///< The sliders for the trigger level
        LevelSlider *markerSlider;          ///< The sliders for the markers
    };

    /// \brief Initializes the components of the oszilloscope-screen.
    /// \param settings The settings object containing the oscilloscope settings.
    /// \param dataAnalyzer The data analyzer that should be used as data source.
    /// \param parent The parent widget.
    /// \param flags Flags for the window manager.
    DsoWidget( DsoSettingsScope *scope, DsoSettingsView *view, const Dso::ControlSpecification *spec, QWidget *parent = nullptr );

    ~DsoWidget() override;

    // Data arrived
    void showNew( std::shared_ptr< PPresult > analysedData );

    void switchToPrintColors();
    void restoreScreenColors();

  protected:
    virtual void showEvent( QShowEvent *event ) override;
    void setupSliders( Sliders &sliders );
    void adaptTriggerLevelSlider( DsoWidget::Sliders &sliders, ChannelID channel );
    void adaptTriggerPositionSlider();
    void setMeasurementVisible( ChannelID channel );
    void updateMarkerDetails();
    void updateSpectrumDetails( ChannelID channel );
    void updateTriggerDetails();
    void updateVoltageDetails( ChannelID channel );

    double mainToZoom( double position ) const;
    double zoomToMain( double position ) const;

    Sliders mainSliders;
    Sliders zoomSliders;

    QGridLayout *mainLayout; ///< The main layout for this widget

    QHBoxLayout *settingsLayout;        ///< The table for the settings info
    QLabel *settingsTriggerLabel;       ///< The trigger details
    QLabel *settingsSamplesOnScreen;    ///< The displayed dots on screen
    QLabel *settingsSamplerateLabel;    ///< The samplerate
    QLabel *settingsTimebaseLabel;      ///< The timebase of the main scope
    QLabel *settingsFrequencybaseLabel; ///< The frequencybase of the main scope

    QLabel *swTriggerStatus; ///< The status of SW trigger

    QHBoxLayout *markerLayout;        ///< The table for the marker details
    QLabel *markerInfoLabel;          ///< The info about the zoom factor
    QLabel *markerTimeLabel;          ///< The time period between the markers
    QLabel *markerFrequencyLabel;     ///< The frequency for the time period
    QLabel *markerTimebaseLabel;      ///< The timebase for the zoomed scope
    QLabel *markerFrequencybaseLabel; ///< The frequencybase for the zoomed scope

    QGridLayout *measurementLayout;                    ///< The table for the signal details
    std::vector< QLabel * > measurementNameLabel;      ///< The name of the channel
    std::vector< QLabel * > measurementGainLabel;      ///< The gain for the voltage (V/div)
    std::vector< QLabel * > measurementMagnitudeLabel; ///< The magnitude for the spectrum (dB/div)
    std::vector< QLabel * > measurementMiscLabel;      ///< Coupling or math mode
    std::vector< QLabel * > measurementVppLabel;       ///< Peak-to-peak amplitude of the signal (V)
    std::vector< QLabel * > measurementRMSLabel;       ///< RMS Amplitude of the signal (V) = sqrt( DC² + AC² )
    std::vector< QLabel * > measurementDCLabel;        ///< DC Amplitude of the signal (V)
    std::vector< QLabel * > measurementACLabel;        ///< AC Amplitude of the signal (V)
    std::vector< QLabel * > measurementdBLabel;        ///< AC Amplitude in dB
    std::vector< QLabel * > measurementFrequencyLabel; ///< Frequency of the signal (Hz)
    std::vector< QLabel * > measurementNoteLabel;      ///< Note value of the signal
    std::vector< QLabel * > measurementRMSPowerLabel;  ///< RMS Power in Watts
    std::vector< QLabel * > measurementTHDLabel;       ///< THD of the signal in Watts

    DataGrid *cursorDataGrid = nullptr;

    DsoSettingsScope *scope;
    DsoSettingsView *view;
    const Dso::ControlSpecification *spec;

    GlScope *mainScope; ///< The main scope screen
    GlScope *zoomScope; ///< The optional magnified scope screen

  private:
    double samplerate;
    double timebase;
    double pulseWidth1 = 0.0;
    double pulseWidth2 = 0.0;
    double zoomFactor = 1.0;
    int mainScopeRow = 0;
    int zoomScopeRow = 0;
    void setColors();
    std::vector< Unit > voltageUnits = { UNIT_VOLTS, UNIT_VOLTS, UNIT_VOLTS };
    bool cursorMeasurementValid = false;
    QPoint cursorGlobalPosition = QPoint();
    QPointF cursorMeasurementPosition = QPointF();
    ChannelID selectedCursor = 0;
    void switchToMarker();
    void showCursorMessage( QPoint globalPos = QPoint(), const QString &message = QString() );
    void updateItem( ChannelID index, bool switchOn = false );

  public slots:
    // Horizontal axis
    // void horizontalFormatChanged(HorizontalFormat format);
    void updateFrequencybase( double frequencybase );
    void updateSamplerate( double samplerate );
    void updateTimebase( double timebase );

    // Trigger
    void updateTriggerMode();
    void updateTriggerSlope();
    void updateTriggerSource();

    // Spectrum
    void updateSpectrumMagnitude( ChannelID channel );
    void updateSpectrumUsed( ChannelID channel, bool used );

    // Vertical axis
    void updateVoltageCoupling( ChannelID channel );
    void updateMathMode();
    void updateVoltageGain( ChannelID channel );
    void updateVoltageUsed( ChannelID channel, bool used );

    // Menus
    void updateRecordLength( int size );

    // Scope control
    void updateZoom( bool enabled );
    void updateCursorGrid( bool enabled );
    void wheelEvent( QWheelEvent *event ) override;

    // Scope control
    void updateSlidersSettings();

  private slots:
    // Sliders
    void updateOffset( ChannelID channel, double value, bool pressed, QPoint globalPos );
    void updateTriggerPosition( int index, double value, bool pressed, QPoint globalPos, bool mainView = true );
    void updateTriggerLevel( ChannelID channel, double value, bool pressed, QPoint globalPos );
    void updateMarker( unsigned marker, double value );

  signals:
    // Sliders
    void voltageOffsetChanged( ChannelID channel, double value ); ///< A graph offset has been changed
    void triggerPositionChanged( double value );                  ///< The pretrigger has been changed
    void triggerLevelChanged( ChannelID channel, double value );  ///< A trigger level has been changed
};
