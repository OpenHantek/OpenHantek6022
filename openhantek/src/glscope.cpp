// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QColor>

#include "glscope.h"

#include "glgenerator.h"
#include "settings.h"

GlScope::GlScope(DsoSettings *settings, const GlGenerator *generator, QWidget *parent)
    : GL_WIDGET_CLASS(parent), settings(settings), generator(generator) {
    connect(generator, &GlGenerator::graphsGenerated, [this]() { update(); });
}

/// \brief Initializes OpenGL output.
void GlScope::initializeGL() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(1);

    QColor bg = settings->view.screen.background;
    glClearColor(bg.redF(), bg.greenF(), bg.blueF(), bg.alphaF());

    glShadeModel(GL_SMOOTH /*GL_FLAT*/);
    glLineStipple(1, 0x3333);

    glEnableClientState(GL_VERTEX_ARRAY);
}

/// \brief Draw the graphs and the grid.
void GlScope::paintGL() {
    // Clear OpenGL buffer and configure settings
    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(1);

    if (settings->view.digitalPhosphorDepth > 0 && generator->isReady()) { drawGraph(); }

    if (!this->zoomed) {
        // Draw vertical lines at marker positions
        glEnable(GL_LINE_STIPPLE);
        QColor trColor = settings->view.screen.markers;
        glColor4f(trColor.redF(), trColor.greenF(), trColor.blueF(), trColor.alphaF());

        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            if (!settings->scope.horizontal.marker_visible[marker]) continue;
            if (vaMarker[marker].size() != 4) {
                vaMarker[marker].resize(2 * 2);
                vaMarker[marker][1] = -DIVS_VOLTAGE;
                vaMarker[marker][3] = DIVS_VOLTAGE;
            }

            vaMarker[marker][0] = settings->scope.horizontal.marker[marker];
            vaMarker[marker][2] = settings->scope.horizontal.marker[marker];

            glVertexPointer(2, GL_FLOAT, 0, &vaMarker[marker].front());
            glDrawArrays(GL_LINES, 0, vaMarker[marker].size() / 2);
        }

        glDisable(GL_LINE_STIPPLE);
    }

    // Draw grid
    this->drawGrid();
}

/// \brief Resize the widget.
/// \param width The new width of the widget.
/// \param height The new height of the widget.
void GlScope::resizeGL(int width, int height) {
    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);

    // Set axes to div-scale and apply correction for exact pixelization
    glLoadIdentity();
    GLdouble pixelizationWidthCorrection = (width > 0) ? (GLdouble)width / (width - 1) : 1;
    GLdouble pixelizationHeightCorrection = (height > 0) ? (GLdouble)height / (height - 1) : 1;
    glOrtho(-(DIVS_TIME / 2) * pixelizationWidthCorrection, (DIVS_TIME / 2) * pixelizationWidthCorrection,
            -(DIVS_VOLTAGE / 2) * pixelizationHeightCorrection, (DIVS_VOLTAGE / 2) * pixelizationHeightCorrection, -1.0,
            1.0);

    glMatrixMode(GL_MODELVIEW);
}

/// \brief Set the zoom mode for this GlScope.
/// \param zoomed true magnifies the area between the markers.
void GlScope::setZoomMode(bool zoomed) { this->zoomed = zoomed; }

/// \brief Draw the grid.
void GlScope::drawGrid() {
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);

    QColor color;
    // Grid
    color = settings->view.screen.grid;
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(0).front());
    glDrawArrays(GL_POINTS, 0, generator->grid(0).size() / 2);
    // Axes
    color = settings->view.screen.axes;
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(1).front());
    glDrawArrays(GL_LINES, 0, generator->grid(1).size() / 2);
    // Border
    color = settings->view.screen.border;
    glColor4f(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(2).front());
    glDrawArrays(GL_LINE_LOOP, 0, generator->grid(2).size() / 2);
}

void GlScope::drawGraphDepth(int mode, int channel, int index) {
    if (generator->channel(mode, channel, index).empty()) return;
    QColor trColor;
    if (mode == Dso::CHANNELMODE_VOLTAGE)
        trColor = settings->view.screen.voltage[channel].darker(fadingFactor[index]);
    else
        trColor = settings->view.screen.spectrum[channel].darker(fadingFactor[index]);
    glColor4f(trColor.redF(), trColor.greenF(), trColor.blueF(), trColor.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->channel(mode, channel, index).front());
    glDrawArrays((settings->view.interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP, 0,
                 generator->channel(mode, channel, index).size() / 2);
}

void GlScope::drawGraph() {
    if (settings->view.antialiasing) {
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }

    // Apply zoom settings via matrix transformation
    if (this->zoomed) {
        glPushMatrix();
        glScalef(DIVS_TIME / fabs(settings->scope.horizontal.marker[1] - settings->scope.horizontal.marker[0]), 1.0,
                 1.0);
        glTranslatef(-(settings->scope.horizontal.marker[0] + settings->scope.horizontal.marker[1]) / 2, 0.0, 0.0);
    }

    // Values we need for the fading of the digital phosphor
    if ((int)fadingFactor.size() != settings->view.digitalPhosphorDepth) {
        fadingFactor.resize((size_t)settings->view.digitalPhosphorDepth);
        fadingFactor[0] = 100;
        double fadingRatio = pow(10.0, 2.0 / settings->view.digitalPhosphorDepth);
        for (size_t index = 1; index < (size_t)settings->view.digitalPhosphorDepth; ++index)
            fadingFactor[index] = fadingFactor[index - 1] * fadingRatio;
    }

    switch (settings->scope.horizontal.format) {
    case Dso::GRAPHFORMAT_TY:
        // Real and virtual channels
        for (int mode = Dso::CHANNELMODE_VOLTAGE; mode < Dso::CHANNELMODE_COUNT; ++mode) {
            for (int channel = 0; channel < settings->scope.voltage.count(); ++channel) {
                if (!channelUsed(mode, channel)) continue;

                // Draw graph for all available depths
                for (int index = settings->view.digitalPhosphorDepth - 1; index >= 0; index--) {
                    drawGraphDepth(mode, channel, index);
                }
            }
        }
        break;

    case Dso::GRAPHFORMAT_XY:
        // Real and virtual channels
        for (int channel = 0; channel < settings->scope.voltage.count() - 1; channel += 2) {
            if (settings->scope.voltage[channel].used) {
                for (int index = settings->view.digitalPhosphorDepth - 1; index >= 0; index--) {
                    drawGraphDepth(Dso::CHANNELMODE_VOLTAGE, channel, index);
                }
            }
        }
        break;

    default:
        break;
    }

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);

    if (this->zoomed) glPopMatrix();
}

bool GlScope::channelUsed(int mode, int channel) {
    return (mode == Dso::CHANNELMODE_VOLTAGE) ? settings->scope.voltage[channel].used
                                              : settings->scope.spectrum[channel].used;
}
