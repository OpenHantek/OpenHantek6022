// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include "exportsettings.h"
#include <QCoreApplication>
#include <QPainter>
#include <QPrinter>
#include <QSize>
#include <memory>

class DsoSettings;
class PPresult;
struct DsoSettingsColorValues;
namespace Dso {
struct ControlSpecification;
}

/// \brief Exports the oscilloscope screen to a file or prints it.
/// TODO
/// Rewrite image exporter with OpenGL drawn grid and graphs
///
/// Sources:
/// https://doc.qt.io/qt-5/qoffscreensurface.html
/// https://doc.qt.io/qt-5/qopenglframebufferobject.html
///
/// https://dangelog.wordpress.com/2013/02/10/using-fbos-instead-of-pbuffers-in-qt-5-2/
class LegacyExportDrawer {
    Q_DECLARE_TR_FUNCTIONS( LegacyExportDrawer )

  public:
    virtual ~LegacyExportDrawer();
    /// Draw the graphs coming from source and labels to the destination paintdevice.
    static bool exportSamples( const PPresult *source, QPaintDevice *dest, const Dso::ControlSpecification *deviceSpecification,
                               const DsoSettings *settings, const DsoSettingsColorValues *colorValues );

  private:
    static void drawGrids( QPainter &painter, const DsoSettingsColorValues *colorValues, double lineHeight, double scopeHeight,
                           int scopeWidth, bool zoom );

    /// Adjust color to be readable on a white background
    static QColor colorForWhiteBackground( const QColor &originalColor );
};
