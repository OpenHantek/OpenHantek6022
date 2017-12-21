// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>

#include <QGLFunctions>
#include <QObject>

#include "analyse/dataanalyzerresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "viewsettings.h"
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
    GlGenerator(DsoSettingsScope *scope, DsoSettingsView *view);
    void generateGraphs(const DataAnalyzerResult *result);
    const std::vector<GLfloat> &channel(int mode, int channel, int index) const;
    const std::vector<GLfloat> &grid(int a) const;
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
