// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <deque>

#include <QGLFunctions>
#include <QObject>

#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"

struct DsoSettingsScope;
class DataAnalyzerResult;
namespace Dso {
struct ControlSpecification;
}

////////////////////////////////////////////////////////////////////////////////
/// \class GlGenerator
/// \brief Generates the vertex arrays for the GlScope classes.
class GlGenerator : public QObject {
    Q_OBJECT

  public:
    /// \brief Initializes the scope widget.
    /// \param settings The target settings object.
    /// \param parent The parent widget.

    GlGenerator();
    bool generateGraphs(const DataAnalyzerResult *result, unsigned digitalPhosphorDepth, const DsoSettingsScope *scope,
                        const Dso::ControlSpecification *spec);
    const std::vector<GLfloat> &channel(Dso::ChannelMode mode, ChannelID channel, unsigned index) const;

    const std::vector<GLfloat> &grid(int a) const;
    bool isReady() const;

  private:
    typedef std::vector<GLfloat> DrawLines;
    typedef std::deque<DrawLines> DrawLinesWithHistory;
    typedef std::vector<DrawLinesWithHistory> DrawLinesWithHistoryPerChannel;
    DrawLinesWithHistoryPerChannel vaChannel[Dso::ChannelModes];
    std::vector<GLfloat> vaGrid[3];
    bool ready = false;
  signals:
    void graphsGenerated(); ///< The graphs are ready to be drawn
};
