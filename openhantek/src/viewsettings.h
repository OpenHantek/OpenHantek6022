#pragma once

#include "definitions.h"
#include <QColor>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QString>

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsColorValues                                    settings.h
/// \brief Holds the color values for the oscilloscope screen.
struct DsoSettingsColorValues {
  QColor axes;            ///< X- and Y-axis and subdiv lines on them
  QColor background;      ///< The scope background
  QColor border;          ///< The border of the scope screen
  QColor grid;            ///< The color of the grid
  QColor markers;         ///< The color of the markers
  QList<QColor> spectrum; ///< The colors of the spectrum graphs
  QColor text;            ///< The default text color
  QList<QColor> voltage;  ///< The colors of the voltage graphs
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsViewColor                                      settings.h
/// \brief Holds the settings for the used colors on the screen and on paper.
struct DsoSettingsViewColor {
  DsoSettingsColorValues screen; ///< Colors for the screen
  DsoSettingsColorValues print;  ///< Colors for printout
};

////////////////////////////////////////////////////////////////////////////////
/// \struct DsoSettingsView                                           settings.h
/// \brief Holds all view settings.
struct DsoSettingsView {
  DsoSettingsViewColor color; ///< Used colors
  bool antialiasing;          ///< Antialiasing for the graphs
  bool digitalPhosphor;       ///< true slowly fades out the previous graphs
  int digitalPhosphorDepth;   ///< Number of channels shown at one time
  Dso::InterpolationMode interpolation; ///< Interpolation mode for the graph
  bool screenColorImages; ///< true exports images with screen colors
  bool zoom;              ///< true if the magnified scope is enabled
};

