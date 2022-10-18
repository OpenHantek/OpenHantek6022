// SPDX-License-Identifier: GPL-2.0-or-later

// increment this value after incompatible config changes
const unsigned CONFIG_VERSION = 3;

#pragma once

#include <QCoreApplication>
#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

#include "post/analysissettings.h"
#include "scopesettings.h"
#include "usb/scopedevice.h"
#include "viewsettings.h"

/// \brief Holds the settings of the program.
class DsoSettings {
    Q_DECLARE_TR_FUNCTIONS( DsoSettings )

  public:
    explicit DsoSettings( const ScopeDevice *scopeDevice, int verboseLevel = 0, bool resetSettings = false );
    bool saveToFile( const QString &filename );
    bool loadFromFile( const QString &filename );

    DsoSettingsScope scope;                  ///< All oscilloscope related settings
    DsoSettingsView view;                    ///< All view related settings
    DsoSettingsAnalysis analysis;            ///< All post processing analysis related settings
    bool exportProcessedSamples = true;      ///< Used for exporting
    bool alwaysSave = true;                  ///< Always save the settings on exit
    unsigned configVersion = CONFIG_VERSION; ///< Handle incompatible changes
    const QString deviceName;                ///< the human readable device name, e.g. DSO-6022BE
    const QString deviceID;                  ///< The unique serial number of EzUSB
    const unsigned deviceFW;                 ///< The FW version number (BCD)

    QByteArray mainWindowGeometry; ///< Geometry of the main window
    QByteArray mainWindowState;    ///< State of docking windows and toolbars

    /// \brief Read the settings from the last session or another file.
    void load();

    /// \brief Save the settings to the harddisk.
    void save();

  private:
    std::unique_ptr< QSettings > storeSettings;
    const Dso::ControlSpecification *deviceSpecification;
    void setDefaultConfig();
    int verboseLevel = 0;
    bool resetSettings = false;
};
