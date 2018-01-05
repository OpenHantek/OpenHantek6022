// SPDX-License-Identifier: GPL-2.0+

#include <cmath>

#include <QColor>

#include "glscope.h"

#include "glgenerator.h"
#include "scopesettings.h"
#include "viewsettings.h"
#include "viewconstants.h"

GlScope *GlScope::createNormal(DsoSettingsScope *scope, DsoSettingsView *view, const GlGenerator *generator, QWidget *parent)
{
    GlScope* s = new GlScope(scope, view, generator, parent);
    s->zoomed = false;
    return s;
}

GlScope *GlScope::createZoomed(DsoSettingsScope *scope, DsoSettingsView *view, const GlGenerator *generator, QWidget *parent)
{
    GlScope* s = new GlScope(scope, view, generator, parent);
    s->zoomed = true;
    return s;
}

GlScope::GlScope(DsoSettingsScope *scope, DsoSettingsView *view, const GlGenerator *generator, QWidget *parent)
    : GL_WIDGET_CLASS(parent), scope(scope), view(view), generator(generator) {
    connect(generator, &GlGenerator::graphsGenerated, [this]() { update(); });
}

void GlScope::initializeGL() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(1);

    QColor bg = view->screen.background;
    glClearColor((GLfloat)bg.redF(), (GLfloat)bg.greenF(), (GLfloat)bg.blueF(), (GLfloat)bg.alphaF());

    glShadeModel(GL_SMOOTH /*GL_FLAT*/);
    glLineStipple(1, 0x3333);

    glEnableClientState(GL_VERTEX_ARRAY);
}

void GlScope::paintGL() {
    // Clear OpenGL buffer and configure settings
    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(1);

    if (generator->isReady()) { drawGraph(view->digitalPhosphorDraws()); }

    if (!this->zoomed) {
        // Draw vertical lines at marker positions
        glEnable(GL_LINE_STIPPLE);
        QColor trColor = view->screen.markers;
        glColor4f((GLfloat)trColor.redF(), (GLfloat)trColor.greenF(), (GLfloat)trColor.blueF(), (GLfloat)trColor.alphaF());

        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            if (!scope->horizontal.marker_visible[marker]) continue;
            if (vaMarker[marker].size() != 4) {
                vaMarker[marker].resize(2 * 2);
                vaMarker[marker][1] = -DIVS_VOLTAGE;
                vaMarker[marker][3] = DIVS_VOLTAGE;
            }

            vaMarker[marker][0] = (GLfloat)scope->horizontal.marker[marker];
            vaMarker[marker][2] = (GLfloat)scope->horizontal.marker[marker];

            glVertexPointer(2, GL_FLOAT, 0, &vaMarker[marker].front());
            glDrawArrays(GL_LINES, 0, (GLsizei)vaMarker[marker].size() / 2);
        }

        glDisable(GL_LINE_STIPPLE);
    }

    // Draw grid
    this->drawGrid();
}

void GlScope::resizeGL(int width, int height) {
    glViewport(0, 0, (GLint)width, (GLint)height);

    glMatrixMode(GL_PROJECTION);

    // Set axes to div-scale and apply correction for exact pixelization
    glLoadIdentity();
    GLdouble pixelizationWidthCorrection = (width > 0) ? (GLdouble)width / (width - 1) : 1;
    GLdouble pixelizationHeightCorrection = (height > 0) ? (GLdouble)height / (height - 1) : 1;
    glOrtho(-(DIVS_TIME / 2.0) * pixelizationWidthCorrection, (DIVS_TIME / 2.0) * pixelizationWidthCorrection,
            -(DIVS_VOLTAGE / 2.0) * pixelizationHeightCorrection, (DIVS_VOLTAGE / 2.0) * pixelizationHeightCorrection, -1.0,
            1.0);

    glMatrixMode(GL_MODELVIEW);
}

void GlScope::drawGrid() {
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);

    QColor color;
    // Grid
    color = view->screen.grid;
    glColor4f((GLfloat)color.redF(), (GLfloat)color.greenF(), (GLfloat)color.blueF(), (GLfloat)color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(0).front());
    glDrawArrays(GL_POINTS, 0, (GLsizei) generator->grid(0).size() / 2);
    // Axes
    color = view->screen.axes;
    glColor4f((GLfloat)color.redF(), (GLfloat)color.greenF(), (GLfloat)color.blueF(), (GLfloat)color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(1).front());
    glDrawArrays(GL_LINES, 0, (GLsizei) generator->grid(1).size() / 2);
    // Border
    color = view->screen.border;
    glColor4f((GLfloat)color.redF(), (GLfloat)color.greenF(), (GLfloat)color.blueF(), (GLfloat)color.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->grid(2).front());
    glDrawArrays(GL_LINE_LOOP, 0, (GLsizei) generator->grid(2).size() / 2);
}

void GlScope::drawGraphDepth(Dso::ChannelMode mode, ChannelID channel, unsigned index) {
    if (generator->channel(mode, channel, index).empty()) return;
    QColor trColor;
    if (mode == Dso::ChannelMode::Voltage)
        trColor = view->screen.voltage[channel].darker(fadingFactor[index]);
    else
        trColor = view->screen.spectrum[channel].darker(fadingFactor[index]);
    glColor4f((GLfloat)trColor.redF(), (GLfloat)trColor.greenF(), (GLfloat)trColor.blueF(), (GLfloat)trColor.alphaF());
    glVertexPointer(2, GL_FLOAT, 0, &generator->channel(mode, channel, index).front());
    glDrawArrays((view->interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP, 0,
                 (GLsizei)generator->channel(mode, channel, index).size() / 2);
}

void GlScope::drawGraph(unsigned digitalPhosphorDepth) {
    if (view->antialiasing) {
        glEnable(GL_POINT_SMOOTH);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }

    // Apply zoom settings via matrix transformation
    if (this->zoomed) {
        glPushMatrix();
        glScalef(DIVS_TIME / (GLfloat)fabs(scope->horizontal.marker[1] - scope->horizontal.marker[0]), 1.0f,
                 1.0f);
        glTranslatef((GLfloat)-(scope->horizontal.marker[0] + scope->horizontal.marker[1]) / 2, 0.0f, 0.0f);
    }

    // Values we need for the fading of the digital phosphor
    if (fadingFactor.size() != digitalPhosphorDepth) {
        fadingFactor.resize(digitalPhosphorDepth);
        fadingFactor[0] = 100;
        double fadingRatio = pow(10.0, 2.0 / digitalPhosphorDepth);
        for (size_t index = 1; index < (size_t)digitalPhosphorDepth; ++index)
            fadingFactor[index] = int(fadingFactor[index - 1] * fadingRatio);
    }

    switch (scope->horizontal.format) {
    case Dso::GraphFormat::TY:
        // Real and virtual channels
        for (Dso::ChannelMode mode: Dso::ChannelModeEnum) {
            for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
                if (!channelUsed(mode, channel)) continue;

                // Draw graph for all available depths
                for (unsigned index = digitalPhosphorDepth; index > 0; index--) {
                    drawGraphDepth(mode, channel, index-1);
                }
            }
        }
        break;
    case Dso::GraphFormat::XY:
        const Dso::ChannelMode mode = Dso::ChannelMode::Voltage;
        // Real and virtual channels
        for (ChannelID channel = 0; channel < scope->voltage.size() - 1; channel += 2) {
            if (!channelUsed(mode, channel)) continue;
            for (unsigned index = digitalPhosphorDepth; index > 0; index--) {
                drawGraphDepth(mode, channel, index-1);
            }
        }
        break;
    }

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);

    if (zoomed) glPopMatrix();
}

bool GlScope::channelUsed(Dso::ChannelMode mode, ChannelID channel) {
    return (mode == Dso::ChannelMode::Voltage) ? scope->voltage[channel].used
                                              : scope->spectrum[channel].used;
}
