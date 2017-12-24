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

#include "definitions.h"

class GlGenerator;
class DsoSettings;

////////////////////////////////////////////////////////////////////////////////
/// \class GlScope                                                     glscope.h
/// \brief OpenGL accelerated widget that displays the oscilloscope screen.
class GlScope : public GL_WIDGET_CLASS {
    Q_OBJECT

  public:
    /// \brief Initializes the scope widget.
    /// \param settings The settings that should be used.
    /// \param parent The parent widget.
    GlScope(DsoSettings *settings, const GlGenerator *generator, QWidget *parent = 0);

    void setZoomMode(bool zoomed);

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void drawGrid();
    void drawGraphDepth(int mode, int channel, int index);
    void drawGraph(int digitalPhosphorDepth);
    bool channelUsed(int mode, int channel);

  private:
    DsoSettings *settings;
    const GlGenerator *generator;
    std::vector<double> fadingFactor;

    std::vector<GLfloat> vaMarker[2];
    bool zoomed = false;
};
