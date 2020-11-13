// SPDX-License-Identifier: GPL-2.0+

// increment this value after incompatible config changes
const unsigned CONFIG_VERSION = 2;

#pragma once

#include <QCoreApplication>
#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

//#include "exporting/exportsettings.h"
#include "post/postprocessingsettings.h"
#include "scopesettings.h"
#include "viewsettings.h"

/// \brief Holds the settings of the program.
class DsoSettings {
    Q_DECLARE_TR_FUNCTIONS( DsoSettings )

  public:
    explicit DsoSettings( const Dso::ControlSpecification *deviceSpecification );
    bool setFilename( const QString &filename );

    DsoSettingsScope scope;                  ///< All oscilloscope related settings
    DsoSettingsView view;                    ///< All view related settings
    DsoSettingsPostProcessing post;          ///< All post processing related settings
    bool exportProcessedSamples = true;      ///< General options of the program
    bool alwaysSave = true;                  ///< Always save the settings on exit
    unsigned configVersion = CONFIG_VERSION; ///< Handle incompatible changes

    QByteArray mainWindowGeometry; ///< Geometry of the main window
    QByteArray mainWindowState;    ///< State of docking windows and toolbars

    /// \brief Read the settings from the last session or another file.
    void load();

    /// \brief Save the settings to the harddisk.
    void save();

  private:
    std::unique_ptr< QSettings > storeSettings = std::unique_ptr< QSettings >( new QSettings );
    const Dso::ControlSpecification *deviceSpecification;
    void setDefaultConfig();
};
