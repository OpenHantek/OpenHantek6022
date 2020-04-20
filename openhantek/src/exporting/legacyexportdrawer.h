// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QPainter>
#include <QPrinter>
#include <QSize>
#include <memory>
#include "exportsettings.h"

class DsoSettings;
class PPresult;
struct DsoSettingsColorValues;
namespace Dso { struct ControlSpecification; }

/// \brief Exports the oscilloscope screen to a file or prints it.
/// TODO
/// Rewrite image exporter with OpenGL drawn grid and graphs
///
/// Sources:
/// http://doc.qt.io/qt-5/qoffscreensurface.html
/// http://doc.qt.io/qt-5/qopenglframebufferobject.html
///
/// https://dangelog.wordpress.com/2013/02/10/using-fbos-instead-of-pbuffers-in-qt-5-2/
class LegacyExportDrawer {
  public:
    /// Draw the graphs coming from source and labels to the destination paintdevice.
    static bool exportSamples(const PPresult *source, QPaintDevice* dest,
                       const Dso::ControlSpecification* deviceSpecification,
                       const DsoSettings *settings, bool isPrinter, const DsoSettingsColorValues *colorValues);

  private:
    static void drawGrids(QPainter &painter, const DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                   int scopeWidth, bool isPrinter, bool zoom);
};
