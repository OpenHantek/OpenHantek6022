// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSettings>
#include <QSize>
#include <QString>
#include <memory>

#include "scopesettings.h"
#include "viewsettings.h"

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptions
/// \brief Holds the general options of the program.
struct DsoSettingsOptions {
    bool alwaysSave = true;            ///< Always save the settings on exit
    QSize imageSize = QSize(640, 480); ///< Size of exported images in pixels
};

////////////////////////////////////////////////////////////////////////////////
/// \class DsoSettings
/// \brief Holds the settings of the program.
class DsoSettings {
  public:
    explicit DsoSettings(unsigned int channels);
    bool setFilename(const QString &filename);

    DsoSettingsOptions options; ///< General options of the program
    DsoSettingsScope scope;     ///< All oscilloscope related settings
    DsoSettingsView view;       ///< All view related settings

    QByteArray mainWindowGeometry; ///< Geometry of the main window
    QByteArray mainWindowState;    ///< State of docking windows and toolbars

    /// \brief Read the settings from the last session or another file.
    void load();

    /// \brief Save the settings to the harddisk.
    void save();

  private:
    std::unique_ptr<QSettings> store = std::unique_ptr<QSettings>(new QSettings);
};
