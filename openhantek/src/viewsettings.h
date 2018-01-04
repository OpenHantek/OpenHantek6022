// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QColor>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QVector>

#include "hantekdso/enums.h"

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsColorValues
/// \brief Holds the color values for the oscilloscope screen.
struct DsoSettingsColorValues {
    QColor axes;                  ///< X- and Y-axis and subdiv lines on them
    QColor background;            ///< The scope background
    QColor border;                ///< The border of the scope screen
    QColor grid;                  ///< The color of the grid
    QColor markers;               ///< The color of the markers
    QColor text;                  ///< The default text color
    std::vector<QColor> spectrum; ///< The colors of the spectrum graphs
    std::vector<QColor> voltage;  ///< The colors of the voltage graphs
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsView
/// \brief Holds all view settings.
struct DsoSettingsView {
    DsoSettingsColorValues screen = {QColor(0xff, 0xff, 0xff, 0x7f), QColor(0x00, 0x00, 0x00, 0xff),
                                     QColor(0xff, 0xff, 0xff, 0xff), QColor(0xff, 0xff, 0xff, 0x3f),
                                     QColor(0xff, 0xff, 0xff, 0xbf), QColor(0xff, 0xff, 0xff, 0xff),
                                     std::vector<QColor>(),          std::vector<QColor>()};
    DsoSettingsColorValues print = {QColor(0x00, 0x00, 0x00, 0xbf), QColor(0x00, 0x00, 0x00, 0x00),
                                    QColor(0x00, 0x00, 0x00, 0xff), QColor(0x00, 0x00, 0x00, 0x7f),
                                    QColor(0x00, 0x00, 0x00, 0xef), QColor(0x00, 0x00, 0x00, 0xff),
                                    std::vector<QColor>(),          std::vector<QColor>()};
    bool antialiasing = true;                                         ///< Antialiasing for the graphs
    bool digitalPhosphor = false;                                     ///< true slowly fades out the previous graphs
    unsigned digitalPhosphorDepth = 8;                                ///< Number of channels shown at one time
    Dso::InterpolationMode interpolation = Dso::INTERPOLATION_LINEAR; ///< Interpolation mode for the graph
    bool screenColorImages = false;                                   ///< true exports images with screen colors
    bool zoom = false;                                                ///< true if the magnified scope is enabled

    unsigned digitalPhosphorDraws() const {
        return digitalPhosphor ? digitalPhosphorDepth : 1;
    }
};
