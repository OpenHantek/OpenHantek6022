// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>

#include <QGLFunctions>
#include <QObject>

#include "scopesettings.h"
#include "viewsettings.h"
#include "dataanalyzerresult.h"

#define DIVS_TIME 10.0   ///< Number of horizontal screen divs
#define DIVS_VOLTAGE 8.0 ///< Number of vertical screen divs
#define DIVS_SUB 5       ///< Number of sub-divisions per div

class GlScope;

////////////////////////////////////////////////////////////////////////////////
/// \class GlGenerator
/// \brief Generates the vertex arrays for the GlScope classes.
class GlGenerator : public QObject {
    Q_OBJECT

public:
    /// \brief Initializes the scope widget.
    /// \param settings The target settings object.
    /// \param parent The parent widget.
    GlGenerator(DsoSettingsScope *scope, DsoSettingsView* view);
    void generateGraphs(const DataAnalyzerResult *result);
    const std::vector<GLfloat>& channel(int mode, int channel, int index) const;
    const std::vector<GLfloat>& grid(int a) const;
    bool isReady() const;
private:
    DsoSettingsScope *settings;
    DsoSettingsView *view;
    std::vector<std::deque<std::vector<GLfloat>>> vaChannel[Dso::CHANNELMODE_COUNT];
    std::vector<GLfloat> vaGrid[3];
    bool ready = false;
signals:
    void graphsGenerated(); ///< The graphs are ready to be drawn
};


