// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <QMetaObject>
#include <QtGlobal>
#include <array>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
#include <QOpenGLWidget>
using GL_WIDGET_CLASS = QOpenGLWidget;
#else
#include <QGLWidget>
using GL_WIDGET_CLASS = QGLWidget;
#endif

#include "hantekdso/enums.h"
#include "hantekprotocol/definitions.h"

class GlGenerator;
class DsoSettings;

/// \brief OpenGL accelerated widget that displays the oscilloscope screen.
class GlScope : public GL_WIDGET_CLASS {
    Q_OBJECT

  public:
    /// \brief Initializes the scope widget.
    /// \param settings The settings that should be used.
    /// \param parent The parent widget.
    GlScope(DsoSettings *settings, const GlGenerator *generator, QWidget *parent = 0);

    void setZoomMode(bool zoomEnabled);

  protected:
    /// \brief Initializes OpenGL output.
    void initializeGL() override;

    /// \brief Draw the graphs and the grid.
    void paintGL() override;

    /// \brief Resize the widget.
    /// \param width The new width of the widget.
    /// \param height The new height of the widget.
    void resizeGL(int width, int height) override;

    /// \brief Draw the grid.
    void drawGrid();

    void drawGraphDepth(Dso::ChannelMode mode, ChannelID channel, unsigned index);
    void drawGraph(unsigned digitalPhosphorDepth);

    /**
     * @brief Return true if the given channel with the given mode is used
     * @param mode The channel mode (spectrum/voltage)
     * @param channel The channel
     */
    bool channelUsed(Dso::ChannelMode mode, ChannelID channel);

  private:
    DsoSettings *settings;
    const GlGenerator *generator;
    std::vector<int> fadingFactor;

    std::vector<GLfloat> vaMarker[2];
    bool zoomed = false;
};
