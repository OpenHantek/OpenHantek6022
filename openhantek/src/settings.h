// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QSize>
#include <QString>

#include "scopesettings.h"
#include "viewsettings.h"

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsOptions                                        settings.h
/// \brief Holds the general options of the program.
struct DsoSettingsOptions {
  bool alwaysSave;                 ///< Always save the settings on exit
  QSize imageSize;                 ///< Size of exported images in pixels
};

////////////////////////////////////////////////////////////////////////////////
/// \class DsoSettings                                                settings.h
/// \brief Holds the settings of the program.
class DsoSettings : public QObject {
  Q_OBJECT

public:
  DsoSettings(QWidget *parent = 0);

  void setChannelCount(unsigned int channels);

  DsoSettingsOptions options; ///< General options of the program
  DsoSettingsScope scope;     ///< All oscilloscope related settings
  DsoSettingsView view;       ///< All view related settings

  QByteArray mainWindowGeometry; ///< Geometry of the main window
  QByteArray mainWindowState;    ///< State of docking windows and toolbars

public slots:
  // Saving to and loading from configuration files
  int load(const QString &fileName = QString());
  int save(const QString &fileName = QString());
};
