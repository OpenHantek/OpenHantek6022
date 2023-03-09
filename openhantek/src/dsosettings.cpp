// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QSettings>

#include "dsosettings.h"
#include "dsowidget.h"
#include "hantekdso/mathmodes.h"

/// \brief Set the number of channels.
/// \param channels The new channel count, that will be applied to lists.
DsoSettings::DsoSettings( const ScopeDevice *scopeDevice, int verboseLevel, bool resetSettings )
    : deviceName( scopeDevice->getModel()->name ), deviceID( scopeDevice->getSerialNumber() ),
      deviceFW( scopeDevice->getFwVersion() ), deviceSpecification( scopeDevice->getModel()->spec() ), verboseLevel( verboseLevel ),
      resetSettings( resetSettings ) {
    scope.verboseLevel = verboseLevel;
    if ( verboseLevel > 1 )
        qDebug() << " DsoSettings::DsoSettings()" << deviceName << deviceID << resetSettings;
    // Add new channels to the list
    int voltage_hue[] = { 60, 210, 0, 120 };   // yellow, lightblue, red, green
    int spectrum_hue[] = { 30, 240, 330, 90 }; // orange, blue, purple, green
    unsigned index = 0;
    scope.hasACcoupling = deviceSpecification->hasACcoupling;
    while ( scope.spectrum.size() < deviceSpecification->channels ) {
        // Spectrum
        DsoSettingsScopeSpectrum newSpectrum;
        newSpectrum.name = tr( "SP%1" ).arg( index + 1 );
        scope.spectrum.push_back( newSpectrum );

        // Voltage
        DsoSettingsScopeVoltage newVoltage;
        newVoltage.name = tr( "CH%1" ).arg( index + 1 );
        scope.voltage.push_back( newVoltage );
        view.screen.voltage.push_back( QColor::fromHsv( voltage_hue[ index ], 0xff, 0xff ) );
        view.screen.spectrum.push_back( QColor::fromHsv( spectrum_hue[ index ], 0xff, 0xff ) );
        view.print.voltage.push_back( view.screen.voltage.back().darker() );
        view.print.spectrum.push_back( view.screen.spectrum.back().darker() );
        if ( ++index >= sizeof voltage_hue )
            index = 0;
    }

    DsoSettingsScopeSpectrum newSpectrum;
    newSpectrum.name = tr( "SPM" );
    scope.spectrum.push_back( newSpectrum );

    DsoSettingsScopeVoltage newVoltage;
    newVoltage.couplingOrMathIndex = unsigned( Dso::MathMode::ADD_CH1_CH2 );
    newVoltage.name = tr( "MATH" );
    scope.voltage.push_back( newVoltage );

    view.screen.voltage.push_back( QColor::fromHsv( 300, 0xff, 0xff ) );  // purple (V=100%)
    view.screen.spectrum.push_back( QColor::fromHsv( 300, 0xff, 0xc0 ) ); // brightness V=75%
    view.print.voltage.push_back( QColor::fromHsv( 300, 0xff, 0xc0 ) );   // brightness V=75%
    view.print.spectrum.push_back( QColor::fromHsv( 300, 0xff, 0x80 ) );  // brightness V=50%

    // create an unique storage for this device based on device name and serial number
    // individual device settings location:
    // Linux, Unix: $HOME/.config/OpenHantek/<deviceName>_<deviceID>.conf
    // macOS:       $HOME/Library/Preferences/org.openhantek.<deviceName>_<deviceID>.plist
    // Windows:     HKEY_CURRENT_USER\Software\OpenHantek\<deviceName>_<deviceID>
    // more info:   https://doc.qt.io/qt-5/qsettings.html#platform-specific-notes
    storeSettings =
        std::unique_ptr< QSettings >( new QSettings( QCoreApplication::organizationName(), deviceName + "_" + deviceID ) );
    // and get the persistent settings
    load();
}


// store the current settings to an explicitly named file
bool DsoSettings::saveToFile( const QString &filename ) {
    if ( verboseLevel > 1 )
        qDebug() << " DsoSettings::saveFilename()" << filename;
    std::unique_ptr< QSettings > local = std::unique_ptr< QSettings >( new QSettings( filename, QSettings::IniFormat ) );
    if ( local->status() != QSettings::NoError ) {
        qWarning() << "Could not save to config file " << filename;
        return false;
    }
    storeSettings.swap( local ); // switch to requested filename
    save();                      // store the settings
    storeSettings.swap( local ); // and switch back to default persistent storage location (file, registry, ...)
    return true;
}


// load settings from a config file
bool DsoSettings::loadFromFile( const QString &filename ) {
    if ( verboseLevel > 1 )
        qDebug() << " DsoSettings::loadFilename()" << filename;
    if ( QFileInfo( filename ).isReadable() ) {
        std::unique_ptr< QSettings > local = std::unique_ptr< QSettings >( new QSettings( filename, QSettings::IniFormat ) );
        if ( local->status() == QSettings::NoError ) {
            storeSettings.swap( local );
            load();
            storeSettings.swap( local );
            return true;
        }
    }
    qWarning() << "Could not load from config file " << filename;
    return false;
}


// load the persistent scope settings
// called by "DsoSettings::DsoSettings()" and "loadFromFile()"
void DsoSettings::load() {
    if ( verboseLevel > 1 )
        qDebug() << " DsoSettings::load()" << storeSettings->fileName();
    // Start with default configuration?
    if ( resetSettings || storeSettings->value( "configuration/version", 0 ).toUInt() < CONFIG_VERSION ) {
        // incompatible change or config reset by user
        storeSettings->clear(); // start with a clean config storage
        QSettings().clear();    // and a clean global storage
        setDefaultConfig();
        return;
    }

    alwaysSave = storeSettings->value( "configuration/alwaysSave", alwaysSave ).toBool();

    // Oscilloscope settings
    storeSettings->beginGroup( "scope" );
    // Horizontal axis
    storeSettings->beginGroup( "horizontal" );
    if ( storeSettings->contains( "format" ) )
        scope.horizontal.format = Dso::GraphFormat( storeSettings->value( "format" ).toInt() );
    if ( storeSettings->contains( "frequencybase" ) )
        scope.horizontal.frequencybase = storeSettings->value( "frequencybase" ).toDouble();
    for ( int marker = 0; marker < 2; ++marker ) {
        QString name;
        name = QString( "marker%1" ).arg( marker );
        if ( storeSettings->contains( name ) )
            scope.setMarker( unsigned( marker ), storeSettings->value( name ).toDouble() );
    }
    if ( storeSettings->contains( "timebase" ) )
        scope.horizontal.timebase = storeSettings->value( "timebase" ).toDouble();
    if ( storeSettings->contains( "maxTimebase" ) )
        scope.horizontal.maxTimebase = storeSettings->value( "maxTimebase" ).toDouble();
    if ( storeSettings->contains( "acquireInterval" ) )
        scope.horizontal.acquireInterval = storeSettings->value( "acquireInterval" ).toDouble();
    if ( storeSettings->contains( "recordLength" ) )
        scope.horizontal.recordLength = storeSettings->value( "recordLength" ).toInt();
    if ( storeSettings->contains( "samplerate" ) )
        scope.horizontal.samplerate = storeSettings->value( "samplerate" ).toDouble();
    if ( storeSettings->contains( "calfreq" ) )
        scope.horizontal.calfreq = storeSettings->value( "calfreq" ).toDouble();
    storeSettings->endGroup(); // horizontal
    // Trigger
    storeSettings->beginGroup( "trigger" );
    if ( storeSettings->contains( "mode" ) )
        scope.trigger.mode = Dso::TriggerMode( storeSettings->value( "mode" ).toUInt() );
    if ( storeSettings->contains( "position" ) )
        scope.trigger.position = storeSettings->value( "position" ).toDouble();
    if ( storeSettings->contains( "slope" ) )
        scope.trigger.slope = Dso::Slope( storeSettings->value( "slope" ).toUInt() );
    if ( storeSettings->contains( "source" ) )
        scope.trigger.source = storeSettings->value( "source" ).toInt();
    if ( storeSettings->contains( "smooth" ) )
        scope.trigger.smooth = storeSettings->value( "smooth" ).toInt();
    storeSettings->endGroup(); // trigger
    // Spectrum
    for ( ChannelID channel = 0; channel < scope.spectrum.size(); ++channel ) {
        storeSettings->beginGroup( QString( "spectrum%1" ).arg( channel ) );
        if ( storeSettings->contains( "magnitude" ) )
            scope.spectrum[ channel ].magnitude = storeSettings->value( "magnitude" ).toDouble();
        if ( storeSettings->contains( "offset" ) )
            scope.spectrum[ channel ].offset = storeSettings->value( "offset" ).toDouble();
        if ( storeSettings->contains( "used" ) )
            scope.spectrum[ channel ].used = storeSettings->value( "used" ).toBool();
        storeSettings->beginGroup( "cursor" );
        if ( storeSettings->contains( "shape" ) )
            scope.spectrum[ channel ].cursor.shape =
                DsoSettingsScopeCursor::CursorShape( storeSettings->value( "shape" ).toUInt() );
        for ( int marker = 0; marker < 2; ++marker ) {
            QString name;
            name = QString( "x%1" ).arg( marker );
            if ( storeSettings->contains( name ) )
                scope.spectrum[ channel ].cursor.pos[ marker ].setX( storeSettings->value( name ).toDouble() );
            name = QString( "y%1" ).arg( marker );
            if ( storeSettings->contains( name ) )
                scope.spectrum[ channel ].cursor.pos[ marker ].setY( storeSettings->value( name ).toDouble() );
        }
        storeSettings->endGroup(); // cursor
        storeSettings->endGroup(); // spectrum%1
    }
    // Voltage
    bool defaultConfig = false;
    // defaultConfig = deviceSpecification->isDemoDevice; // use default channel setting in demo mode
    if ( storeSettings->contains( "hasACmodification" ) )
        scope.hasACmodification = storeSettings->value( "hasACmodification" ).toBool();
    for ( ChannelID channel = 0; channel < scope.voltage.size(); ++channel ) {
        storeSettings->beginGroup( QString( "voltage%1" ).arg( channel ) );
        if ( storeSettings->contains( "gainStepIndex" ) )
            scope.voltage[ channel ].gainStepIndex = storeSettings->value( "gainStepIndex" ).toUInt();
        if ( storeSettings->contains( "couplingOrMathIndex" ) ) {
            scope.voltage[ channel ].couplingOrMathIndex = storeSettings->value( "couplingOrMathIndex" ).toUInt();
            if ( channel < deviceSpecification->channels ) {
                if ( scope.voltage[ channel ].couplingOrMathIndex >= deviceSpecification->couplings.size() ||
                     ( !scope.hasACcoupling && !scope.hasACmodification ) )
                    scope.voltage[ channel ].couplingOrMathIndex = 0; // set to default if out of range
            } else {
                if ( scope.voltage[ channel ].couplingOrMathIndex > unsigned( Dso::LastMathMode ) )
                    scope.voltage[ channel ].couplingOrMathIndex = 0;
            }
        }
        if ( storeSettings->contains( "inverted" ) )
            scope.voltage[ channel ].inverted = storeSettings->value( "inverted" ).toBool();
        if ( storeSettings->contains( "offset" ) )
            scope.voltage[ channel ].offset = storeSettings->value( "offset" ).toDouble();
        if ( storeSettings->contains( "trigger" ) )
            scope.voltage[ channel ].trigger = storeSettings->value( "trigger" ).toDouble();
        if ( storeSettings->contains( "probeAttn" ) )
            scope.voltage[ channel ].probeAttn = storeSettings->value( "probeAttn" ).toDouble();
        if ( storeSettings->contains( "used" ) )
            scope.voltage[ channel ].used = storeSettings->value( "used" ).toBool();
        else                      // no config file found, e.g. 1st run
            defaultConfig = true; // start with default config

        if ( defaultConfig ) { // useful default: show both voltage channels
            setDefaultConfig();
        }

        storeSettings->beginGroup( "cursor" );
        if ( storeSettings->contains( "shape" ) )
            scope.voltage[ channel ].cursor.shape = DsoSettingsScopeCursor::CursorShape( storeSettings->value( "shape" ).toUInt() );
        for ( int marker = 0; marker < 2; ++marker ) {
            QString name;
            name = QString( "x%1" ).arg( marker );
            if ( storeSettings->contains( name ) )
                scope.voltage[ channel ].cursor.pos[ marker ].setX( storeSettings->value( name ).toDouble() );
            name = QString( "y%1" ).arg( marker );
            if ( storeSettings->contains( name ) )
                scope.voltage[ channel ].cursor.pos[ marker ].setY( storeSettings->value( name ).toDouble() );
        }
        storeSettings->endGroup(); // cursor
        storeSettings->endGroup(); // voltage%1
    }

    // Post processing
    if ( storeSettings->contains( "spectrumLimit" ) )
        analysis.spectrumLimit = storeSettings->value( "spectrumLimit" ).toDouble();
    if ( storeSettings->contains( "spectrumWindow" ) ) {
        analysis.spectrumWindow = Dso::WindowFunction( storeSettings->value( "spectrumWindow" ).toInt() );
        if ( analysis.spectrumWindow > Dso::LastWindowFunction )
            analysis.spectrumWindow = Dso::WindowFunction::HAMMING; // fall back to something useful
    }
    // Analysis
    storeSettings->beginGroup( "analysis" );
    if ( storeSettings->contains( "spectrumReference" ) )
        scope.analysis.spectrumReference = storeSettings->value( "spectrumReference" ).toDouble();
    if ( storeSettings->contains( "dBsuffixIndex" ) )
        scope.analysis.dBsuffixIndex = storeSettings->value( "dBsuffixIndex" ).toInt();
    if ( storeSettings->contains( "calculateDummyLoad" ) )
        scope.analysis.calculateDummyLoad = storeSettings->value( "calculateDummyLoad" ).toBool();
    if ( storeSettings->contains( "dummyLoad" ) )
        scope.analysis.dummyLoad = storeSettings->value( "dummyLoad" ).toUInt();
    if ( storeSettings->contains( "calculateTHD" ) )
        scope.analysis.calculateTHD = storeSettings->value( "calculateTHD" ).toBool();
    if ( storeSettings->contains( "reuseFftPlan" ) )
        analysis.reuseFftPlan = storeSettings->value( "reuseFftPlan" ).toBool();
    if ( storeSettings->contains( "showNoteValue" ) )
        scope.analysis.showNoteValue = storeSettings->value( "showNoteValue" ).toBool();
    storeSettings->endGroup(); // analysis
    storeSettings->endGroup(); // scope

    // View
    storeSettings->beginGroup( "view" );
    // Colors
    storeSettings->beginGroup( "color" );
    DsoSettingsColorValues *colors;
    for ( int mode = 0; mode < 2; ++mode ) {
        if ( mode == 0 ) {
            colors = &view.screen;
            storeSettings->beginGroup( "screen" );
        } else {
            colors = &view.print;
            storeSettings->beginGroup( "print" );
        }

        if ( storeSettings->contains( "axes" ) )
            colors->axes = storeSettings->value( "axes" ).value< QColor >();
        if ( storeSettings->contains( "background" ) )
            colors->background = storeSettings->value( "background" ).value< QColor >();
        if ( storeSettings->contains( "border" ) )
            colors->border = storeSettings->value( "border" ).value< QColor >();
        if ( storeSettings->contains( "grid" ) )
            colors->grid = storeSettings->value( "grid" ).value< QColor >();
        if ( storeSettings->contains( "markers" ) )
            colors->markers = storeSettings->value( "markers" ).value< QColor >();
        for ( ChannelID channel = 0; channel < scope.spectrum.size(); ++channel ) {
            QString key = QString( "spectrum%1" ).arg( channel );
            if ( storeSettings->contains( key ) )
                colors->spectrum[ channel ] = storeSettings->value( key ).value< QColor >();
        }
        if ( storeSettings->contains( "text" ) )
            colors->text = storeSettings->value( "text" ).value< QColor >();
        for ( ChannelID channel = 0; channel < scope.voltage.size(); ++channel ) {
            QString key = QString( "voltage%1" ).arg( channel );
            if ( storeSettings->contains( key ) )
                colors->voltage[ channel ] = storeSettings->value( key ).value< QColor >();
        }
        storeSettings->endGroup(); // screen / print
    }
    storeSettings->endGroup(); // color
    // Other view settings
    if ( storeSettings->contains( "histogram" ) )
        scope.histogram = storeSettings->value( "histogram" ).toBool();
    if ( storeSettings->contains( "digitalPhosphor" ) )
        view.digitalPhosphor = storeSettings->value( "digitalPhosphor" ).toBool();
    if ( storeSettings->contains( "interpolation" ) )
        view.interpolation = Dso::InterpolationMode( storeSettings->value( "interpolation" ).toInt() );
    if ( storeSettings->contains( "printerColorImages" ) )
        view.printerColorImages = storeSettings->value( "printerColorImages" ).toBool();
    if ( storeSettings->contains( "zoom" ) )
        view.zoom = storeSettings->value( "zoom" ).toBool();
    if ( storeSettings->contains( "zoomHeightIndex" ) )
        view.zoomHeightIndex = storeSettings->value( "zoomHeightIndex" ).toInt();
    if ( storeSettings->contains( "zoomImage" ) )
        view.zoomImage = storeSettings->value( "zoomImage" ).toBool();
    if ( storeSettings->contains( "exportScaleValue" ) )
        view.exportScaleValue = storeSettings->value( "exportScaleValue" ).toInt();
    if ( storeSettings->contains( "cursorGridPosition" ) )
        view.cursorGridPosition = Qt::ToolBarArea( storeSettings->value( "cursorGridPosition" ).toUInt() );
    if ( storeSettings->contains( "cursorsVisible" ) )
        view.cursorsVisible = storeSettings->value( "cursorsVisible" ).toBool();
    storeSettings->endGroup(); // view

    storeSettings->beginGroup( "window" );
    mainWindowGeometry = storeSettings->value( "geometry" ).toByteArray();
    mainWindowState = storeSettings->value( "state" ).toByteArray();
    storeSettings->endGroup(); // window
}


// save the persistent scope settings
// called by "DsoSettings::saveToFile()", "MainWindow::closeEvent" and explicitly by "ui->actionSave"
void DsoSettings::save() {
    // Use default configuration after restart?
    if ( 0 == configVersion ) {
        storeSettings->clear();
        if ( verboseLevel > 1 )
            qDebug() << " DsoSettings::save() storeSettings->clear() << storeSettings->fileName()";
        return;
    } else { // save fontSize as global setting
        QSettings().setValue( "view/fontSize", view.fontSize );
        QSettings().setValue( "view/toolTipVisible", scope.toolTipVisible );
    }
    if ( verboseLevel > 1 )
        qDebug() << " DsoSettings::save()" << storeSettings->fileName();
    // now store individual device values

    // Date and Time of last storage
    storeSettings->beginGroup( "ConfigurationSaved" );
    storeSettings->setValue( "Date", QDate::currentDate().toString( "yyyy-MM-dd" ) );
    storeSettings->setValue( "Time", QTime::currentTime().toString( "HH:mm:ss" ) );
    storeSettings->endGroup(); // ConfigurationSaved

    // Device ID (helps to identify the connection of a "Save as" file with a specific device)
    storeSettings->beginGroup( "DeviceID" );
    storeSettings->setValue( "Model", deviceName );
    storeSettings->setValue( "SerialNumber", deviceID );
    storeSettings->endGroup(); // DeviceID

    // Configuration settings
    storeSettings->beginGroup( "configuration" );
    storeSettings->setValue( "version", configVersion );
    storeSettings->setValue( "alwaysSave", alwaysSave );
    storeSettings->endGroup(); // configuration

    // Oszilloskope settings
    storeSettings->beginGroup( "scope" );
    // Horizontal axis
    storeSettings->beginGroup( "horizontal" );
    storeSettings->setValue( "format", scope.horizontal.format );
    storeSettings->setValue( "frequencybase", scope.horizontal.frequencybase );
    for ( int marker = 0; marker < 2; ++marker )
        storeSettings->setValue( QString( "marker%1" ).arg( marker ), scope.getMarker( marker ) );
    storeSettings->setValue( "timebase", scope.horizontal.timebase );
    storeSettings->setValue( "maxTimebase", scope.horizontal.maxTimebase );
    storeSettings->setValue( "acquireInterval", scope.horizontal.acquireInterval );
    storeSettings->setValue( "recordLength", scope.horizontal.recordLength );
    storeSettings->setValue( "samplerate", scope.horizontal.samplerate );
    storeSettings->setValue( "calfreq", scope.horizontal.calfreq );
    storeSettings->endGroup(); // horizontal
    // Trigger
    storeSettings->beginGroup( "trigger" );
    storeSettings->setValue( "mode", unsigned( scope.trigger.mode ) );
    storeSettings->setValue( "position", scope.trigger.position );
    storeSettings->setValue( "slope", unsigned( scope.trigger.slope ) );
    storeSettings->setValue( "source", scope.trigger.source );
    storeSettings->setValue( "smooth", scope.trigger.smooth );
    storeSettings->endGroup(); // trigger
    // Spectrum
    for ( ChannelID channel = 0; channel < scope.spectrum.size(); ++channel ) {
        storeSettings->beginGroup( QString( "spectrum%1" ).arg( channel ) );
        storeSettings->setValue( "magnitude", scope.spectrum[ channel ].magnitude );
        storeSettings->setValue( "offset", scope.spectrum[ channel ].offset );
        storeSettings->setValue( "used", scope.spectrum[ channel ].used );
        storeSettings->beginGroup( "cursor" );
        storeSettings->setValue( "shape", scope.spectrum[ channel ].cursor.shape );
        for ( int marker = 0; marker < 2; ++marker ) {
            QString name;
            name = QString( "x%1" ).arg( marker );
            storeSettings->setValue( name, scope.spectrum[ channel ].cursor.pos[ marker ].x() );
            name = QString( "y%1" ).arg( marker );
            storeSettings->setValue( name, scope.spectrum[ channel ].cursor.pos[ marker ].y() );
        }
        storeSettings->endGroup(); // cursor
        storeSettings->endGroup(); // spectrum%1
    }
    // Voltage
    storeSettings->setValue( "hasACmodification", scope.hasACmodification );
    for ( ChannelID channel = 0; channel < scope.voltage.size(); ++channel ) {
        storeSettings->beginGroup( QString( "voltage%1" ).arg( channel ) );
        storeSettings->setValue( "gainStepIndex", scope.voltage[ channel ].gainStepIndex );
        storeSettings->setValue( "couplingOrMathIndex", scope.voltage[ channel ].couplingOrMathIndex );
        storeSettings->setValue( "inverted", scope.voltage[ channel ].inverted );
        storeSettings->setValue( "offset", scope.voltage[ channel ].offset );
        storeSettings->setValue( "trigger", scope.voltage[ channel ].trigger );
        storeSettings->setValue( "used", scope.voltage[ channel ].used );
        storeSettings->setValue( "probeAttn", scope.voltage[ channel ].probeAttn );
        storeSettings->beginGroup( "cursor" );
        storeSettings->setValue( "shape", scope.voltage[ channel ].cursor.shape );
        for ( int marker = 0; marker < 2; ++marker ) {
            QString name;
            name = QString( "x%1" ).arg( marker );
            storeSettings->setValue( name, scope.voltage[ channel ].cursor.pos[ marker ].x() );
            name = QString( "y%1" ).arg( marker );
            storeSettings->setValue( name, scope.voltage[ channel ].cursor.pos[ marker ].y() );
        }
        storeSettings->endGroup(); // cursor
        storeSettings->endGroup(); // voltage%1
    }

    // Post processing
    storeSettings->setValue( "spectrumLimit", analysis.spectrumLimit );
    storeSettings->setValue( "spectrumWindow", unsigned( analysis.spectrumWindow ) );

    // Analysis
    storeSettings->beginGroup( "analysis" );
    storeSettings->setValue( "spectrumReference", scope.analysis.spectrumReference );
    storeSettings->setValue( "dBsuffixIndex", scope.analysis.dBsuffixIndex );
    storeSettings->setValue( "calculateDummyLoad", scope.analysis.calculateDummyLoad );
    storeSettings->setValue( "dummyLoad", scope.analysis.dummyLoad );
    storeSettings->setValue( "calculateTHD", scope.analysis.calculateTHD );
    storeSettings->setValue( "reuseFftPlan", analysis.reuseFftPlan );
    storeSettings->setValue( "showNoteValue", scope.analysis.showNoteValue );
    storeSettings->endGroup(); // analysis
    storeSettings->endGroup(); // scope

    // View
    storeSettings->beginGroup( "view" );
    // Colors
    storeSettings->beginGroup( "color" );
    DsoSettingsColorValues *colors;
    for ( int mode = 0; mode < 2; ++mode ) {
        if ( mode == 0 ) {
            colors = &view.screen;
            storeSettings->beginGroup( "screen" );
        } else {
            colors = &view.print;
            storeSettings->beginGroup( "print" );
        }

        storeSettings->setValue( "axes", colors->axes.name( QColor::HexArgb ) );
        storeSettings->setValue( "background", colors->background.name( QColor::HexArgb ) );
        storeSettings->setValue( "border", colors->border.name( QColor::HexArgb ) );
        storeSettings->setValue( "grid", colors->grid.name( QColor::HexArgb ) );
        storeSettings->setValue( "markers", colors->markers.name( QColor::HexArgb ) );
        for ( ChannelID channel = 0; channel < scope.spectrum.size(); ++channel )
            storeSettings->setValue( QString( "spectrum%1" ).arg( channel ), colors->spectrum[ channel ].name( QColor::HexArgb ) );
        storeSettings->setValue( "text", colors->text.name( QColor::HexArgb ) );
        for ( ChannelID channel = 0; channel < scope.voltage.size(); ++channel )
            storeSettings->setValue( QString( "voltage%1" ).arg( channel ), colors->voltage[ channel ].name( QColor::HexArgb ) );
        storeSettings->endGroup(); // screen / print
    }
    storeSettings->endGroup(); // color

    // Other view settings
    storeSettings->setValue( "histogram", scope.histogram );
    storeSettings->setValue( "digitalPhosphor", view.digitalPhosphor );
    storeSettings->setValue( "interpolation", view.interpolation );
    storeSettings->setValue( "printerColorImages", view.printerColorImages );
    storeSettings->setValue( "zoom", view.zoom );
    storeSettings->setValue( "zoomHeightIndex", view.zoomHeightIndex );
    storeSettings->setValue( "zoomImage", view.zoomImage );
    storeSettings->setValue( "exportScaleValue", view.exportScaleValue );
    storeSettings->setValue( "cursorGridPosition", view.cursorGridPosition );
    storeSettings->setValue( "cursorsVisible", view.cursorsVisible );
    storeSettings->endGroup(); // view

    // Program window geometry and state
    storeSettings->beginGroup( "window" );
    storeSettings->setValue( "geometry", mainWindowGeometry );
    storeSettings->setValue( "state", mainWindowState );
    storeSettings->endGroup(); // window
}


void DsoSettings::setDefaultConfig() {
    scope.voltage[ 0 ].used = true;
    scope.voltage[ 0 ].offset = MARGIN_TOP / 2; // mid of upper screen half
    scope.voltage[ 1 ].used = true;
    scope.voltage[ 1 ].offset = MARGIN_BOTTOM / 2; // mid of lower screen half
}
