// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QVector>

#include "hantekdso/enums.h"

// These values allow a quite narrow but readable display
const QString defaultFont = "Arial";
const int defaultFontSize = 10;
const int defaultCondensed = 87; // SemiCondensed = 87%

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsColorValues
/// \brief Holds the color values for the oscilloscope screen.
struct DsoSettingsColorValues {
    QColor axes;                    ///< X- and Y-axis and subdiv lines on them
    QColor background;              ///< The scope background
    QColor border;                  ///< The border of the scope screen
    QColor grid;                    ///< The color of the grid
    QColor markers;                 ///< The color of the markers
    QColor text;                    ///< The default text color
    std::vector< QColor > spectrum; ///< The colors of the spectrum graphs
    std::vector< QColor > voltage;  ///< The colors of the voltage graphs
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsView
/// \brief Holds all view settings.
struct DsoSettingsView {
    DsoSettingsColorValues screen = { QColor( 0x7f, 0x7f, 0x7f, 0xff ), QColor( 0x00, 0x00, 0x00, 0xff ), // axes, background
                                      QColor( 0xff, 0xff, 0xff, 0xff ), QColor( 0xc0, 0xc0, 0xc0, 0xff ), // border, grid
                                      QColor( 0xc0, 0xc0, 0xc0, 0xff ), QColor( 0xff, 0xff, 0xff, 0xff ), // markers, text
                                      std::vector< QColor >(),          std::vector< QColor >() };        // spectrum, voltage
    DsoSettingsColorValues print = { QColor( 0x40, 0x40, 0x40, 0xff ), QColor( 0xff, 0xff, 0xff, 0xff ),  // axes, background
                                     QColor( 0x00, 0x00, 0x00, 0xff ), QColor( 0x40, 0x40, 0x40, 0xff ),  // border, grid
                                     QColor( 0x40, 0x40, 0x40, 0xff ), QColor( 0x00, 0x00, 0x00, 0xff ),  // markers, text
                                     std::vector< QColor >(),          std::vector< QColor >() };         // spectrum, voltage
    bool antialiasing = true;                                         ///< Antialiasing for the graphs
    bool digitalPhosphor = false;                                     ///< true slowly fades out the previous graphs
    unsigned digitalPhosphorDepth = 8;                                ///< Number of channels shown at one time
    Dso::InterpolationMode interpolation = Dso::INTERPOLATION_LINEAR; ///< Interpolation mode for the graph
    bool printerColorImages = true;                                   ///< Exports images with screen colors
    int zoomHeightIndex = 2;                                          ///< Zoom scope window height
    bool zoomImage = true;                                            ///< Export zoomed images with double height
    bool zoom = false;                                                ///< true if the magnified scope is enabled
    int exportScaleValue = 1;
    Qt::ToolBarArea cursorGridPosition = Qt::RightToolBarArea;
    bool cursorsVisible = false;
    DsoSettingsColorValues *colors = &screen;
    int fontSize = defaultFontSize;
    bool styleFusion = false;
    int theme = 0;
    unsigned screenHeight = 0;
    unsigned screenWidth = 0;

    unsigned digitalPhosphorDraws() const { return digitalPhosphor ? digitalPhosphorDepth : 1; }
};
