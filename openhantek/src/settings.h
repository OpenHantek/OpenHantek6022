// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

#include "scopesettings.h"
#include "viewsettings.h"
#include "exporting/exportsettings.h"
#include "hantekdso/controlspecification.h"
#include "hantekdso/controlsettings.h"

/// \brief Holds the settings of the program.
class DsoSettings {
  public:
    explicit DsoSettings(const Dso::ControlSpecification* deviceSpecification);
    bool setFilename(const QString &filename);

    DsoSettingsExport exporting; ///< General options of the program
    DsoSettingsScope scope;     ///< All oscilloscope related settings
    DsoSettingsView view;       ///< All view related settings
    bool alwaysSave = true;            ///< Always save the settings on exit

    QByteArray mainWindowGeometry; ///< Geometry of the main window
    QByteArray mainWindowState;    ///< State of docking windows and toolbars

    /// \brief Read the settings from the last session or another file.
    void load();

    /// \brief Save the settings to the harddisk.
    void save();

  private:
    std::unique_ptr<QSettings> store = std::unique_ptr<QSettings>(new QSettings);
};
