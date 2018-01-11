// SPDX-License-Identifier: GPL-2.0+

#include <cmath>
#include <iostream>

#include <QColor>
#include <QDebug>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFunctions_ES2>

#include "glscope.h"

#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "viewsettings.h"

// typedef QOpenGLFunctions_ES2 OPENGL_VER;
typedef QOpenGLFunctions_3_3_Core OPENGL_VER;

GlScope *GlScope::createNormal(DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent) {
    GlScope *s = new GlScope(scope, view, parent);
    s->zoomed = false;
    return s;
}

GlScope *GlScope::createZoomed(DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent) {
    GlScope *s = new GlScope(scope, view, parent);
    s->zoomed = true;
    return s;
}

GlScope::GlScope(DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent)
    : QOpenGLWidget(parent), scope(scope), view(view) {
    vaMarker.resize(MARKER_COUNT);
}

GlScope::~GlScope() {}

void GlScope::mousePressEvent(QMouseEvent *event) {
    if (!zoomed && event->button() == Qt::LeftButton) {
        double position = (double)(event->x() - width() / 2) * DIVS_TIME / (double)width();
        double distance = DIVS_TIME;
        selectedMarker = NO_MARKER;
        // Capture nearest marker located within snap area (+/- 1% of full scale).
        for (int marker = 0; marker < MARKER_COUNT; ++marker) {
            if (!scope->horizontal.marker_visible[marker]) continue;
            if (fabs(scope->horizontal.marker[marker] - position) < std::min(distance, DIVS_TIME / 100.0)) {
                distance = fabs(scope->horizontal.marker[marker] - position);
                selectedMarker = marker;
            }
        }
        if (selectedMarker != NO_MARKER) { emit markerMoved(selectedMarker, position); }
    }
    event->accept();
}

void GlScope::mouseMoveEvent(QMouseEvent *event) {
    if (!zoomed && (event->buttons() & Qt::LeftButton) != 0) {
        double position = (double)(event->x() - width() / 2) * DIVS_TIME / (double)width();
        if (selectedMarker == NO_MARKER) {
            // User started draging outside the snap area of any marker:
            // move all markers to current position and select last marker in the array.
            for (unsigned marker = 0; marker < MARKER_COUNT; ++marker) {
                emit markerMoved(marker, position);
                selectedMarker = marker;
            }
        } else if (selectedMarker < MARKER_COUNT) {
            emit markerMoved(selectedMarker, position);
        }
    }
    event->accept();
}

void GlScope::mouseReleaseEvent(QMouseEvent *event) {
    if (!zoomed && event->button() == Qt::LeftButton) {
        double position = (double)(event->x() - width() / 2) * DIVS_TIME / (double)width();
        if (selectedMarker < MARKER_COUNT) { emit markerMoved(selectedMarker, position); }
        selectedMarker = NO_MARKER;
    }
    event->accept();
}

void GlScope::initializeGL() {
    auto *gl = context()->versionFunctions<OPENGL_VER>();

    m_program = std::unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram(context()));

    const char *vshaderES = R"(
          #version 330
          attribute highp vec3 vertex;
          uniform mat4 matrix;
          void main()
          {
              gl_Position = matrix * vec4(vertex, 1.0);
              gl_PointSize = 1.0;
          }
    )";

    const char *vshaderCore = R"(
          #version 330
          in highp vec3 vertex;
          uniform mat4 matrix;
          void main()
          {
              gl_Position = matrix * vec4(vertex, 1.0);
              gl_PointSize = 1.0;
          }
    )";
    const char *fshaderCore = R"(
          #version 330
          uniform highp vec4 colour;
          void main() { gl_FragColor = colour; }
    )";

    qDebug() << "compile shaders";
    // Compile vertex shader
    bool coreShaders = QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile;
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, coreShaders ? vshaderCore : vshaderES) ||
        !m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fshaderCore)) {
        qWarning() << "Failed to compile OpenGL shader programs.\n" << m_program->log();
        return;
    }

    // Link shader pipeline
    if (!m_program->link() || !m_program->bind()) {
        qWarning() << "Failed to link/bind OpenGL shader programs";
        return;
    }

    vertexLocation = m_program->attributeLocation("vertex");
    matrixLocation = m_program->uniformLocation("matrix");
    colorLocation = m_program->uniformLocation("colour");

    if (vertexLocation == -1 || colorLocation == -1 || matrixLocation == -1) {
        qWarning() << "Failed to locate shader variable";
        return;
    }

    m_program->bind();

    gl->glDisable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    // Enable depth buffer
    gl->glEnable(GL_DEPTH_TEST);

    // Enable back face culling
    gl->glEnable(GL_CULL_FACE);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gl->glPointSize(1);

    QColor bg = view->screen.background;
    gl->glClearColor((GLfloat)bg.redF(), (GLfloat)bg.greenF(), (GLfloat)bg.blueF(), (GLfloat)bg.alphaF());

    generateGrid();

    {
        m_vaoMarker.create();
        QOpenGLVertexArrayObject::Binder b(&m_vaoMarker);
        m_marker.create();
        m_marker.bind();
        m_marker.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_marker.allocate(4 * sizeof(QVector3D));
        m_program->enableAttributeArray(vertexLocation);
        m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 0);
    }

    m_program->release();
}

void GlScope::showData(PPresult *data) {
    if (!m_program || !m_program->isLinked()) return;
    makeCurrent();
    m_GraphHistory.resize(view->digitalPhosphorDraws());
    m_GraphHistory[currentGraphInHistory].writeData(data, m_program.get(), vertexLocation);
    currentGraphInHistory = (currentGraphInHistory + 1) % m_GraphHistory.size();
    doneCurrent();
}

void GlScope::paintGL() {
    if (!m_program->isLinked()) return;

    auto *gl = context()->versionFunctions<OPENGL_VER>();

    // Clear OpenGL buffer and configure settings
    // TODO Don't clear if view->digitalPhosphorDraws()>1
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl->glLineWidth(1);

    m_program->bind();

    if (view->antialiasing) {
        gl->glEnable(GL_POINT_SMOOTH);
        gl->glEnable(GL_LINE_SMOOTH);
        gl->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    }

    // Apply zoom settings via matrix transformation
    if (zoomed) {
        QMatrix4x4 m;
        m.scale(QVector3D(DIVS_TIME / (GLfloat)fabs(scope->horizontal.marker[1] - scope->horizontal.marker[0]), 1.0f,
                          1.0f));
        m.translate((GLfloat) - (scope->horizontal.marker[0] + scope->horizontal.marker[1]) / 2, 0.0f, 0.0f);
        m_program->setUniformValue(matrixLocation, pmvMatrix * m);
    }

    for (unsigned historyIndex = 0; historyIndex < m_GraphHistory.size(); ++historyIndex) {
        unsigned graphID = (historyIndex + currentGraphInHistory) % m_GraphHistory.size();
        Graph &graph = m_GraphHistory[graphID];
        for (ChannelID channel = 0; channel < scope->voltage.size(); ++channel) {
            drawSpectrumChannelGraph(channel, graph, (int)historyIndex);
            drawVoltageChannelGraph(channel, graph, (int)historyIndex);
        }
    }

    gl->glDisable(GL_POINT_SMOOTH);
    gl->glDisable(GL_LINE_SMOOTH);

    if (zoomed) { m_program->setUniformValue(matrixLocation, pmvMatrix); }

    if (!this->zoomed) drawMarkers();

    drawGrid();
    m_program->release();
}

void GlScope::resizeGL(int width, int height) {
    auto *gl = context()->versionFunctions<OPENGL_VER>();
    gl->glViewport(0, 0, (GLint)width, (GLint)height);

    // Set axes to div-scale and apply correction for exact pixelization
    float pixelizationWidthCorrection = (float)width / (width - 1);
    float pixelizationHeightCorrection = (float)height / (height - 1);

    pmvMatrix.setToIdentity();
    pmvMatrix.ortho(-(DIVS_TIME / 2.0f) * pixelizationWidthCorrection, (DIVS_TIME / 2.0f) * pixelizationWidthCorrection,
                    -(DIVS_VOLTAGE / 2.0f) * pixelizationHeightCorrection,
                    (DIVS_VOLTAGE / 2.0f) * pixelizationHeightCorrection, -1.0f, 1.0f);

    m_program->bind();
    m_program->setUniformValue(matrixLocation, pmvMatrix);
    m_program->release();
}

void GlScope::drawGrid() {
    auto *gl = context()->versionFunctions<OPENGL_VER>();
    gl->glDisable(GL_POINT_SMOOTH);
    gl->glDisable(GL_LINE_SMOOTH);
    gl->glLineWidth(1);

    // Grid
    m_vaoGrid[0].bind();
    m_program->setUniformValue(colorLocation, view->screen.grid);
    gl->glDrawArrays(GL_POINTS, 0, gridDrawCounts[0]);
    m_vaoGrid[0].release();

    // Axes
    m_vaoGrid[1].bind();
    m_program->setUniformValue(colorLocation, view->screen.axes);
    gl->glDrawArrays(GL_LINES, 0, gridDrawCounts[1]);
    m_vaoGrid[1].release();

    // Border
    m_vaoGrid[2].bind();
    m_program->setUniformValue(colorLocation, view->screen.border);
    gl->glDrawArrays(GL_LINE_LOOP, 0, gridDrawCounts[2]);
    m_vaoGrid[2].release();
}

void GlScope::generateGrid() {
    gridDrawCounts[0] = 0;
    gridDrawCounts[1] = 0;
    gridDrawCounts[2] = 0;

    m_grid.create();
    m_grid.bind();
    m_grid.setUsagePattern(QOpenGLBuffer::StaticDraw);

    std::vector<QVector3D> vaGrid;

    { // Bind draw vertical lines
        m_vaoGrid[0].create();
        QOpenGLVertexArrayObject::Binder b(&m_vaoGrid[0]);
        m_grid.bind();
        m_program->enableAttributeArray(vertexLocation);
        m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, 0);
    }

    // Draw vertical lines
    for (int div = 1; div < DIVS_TIME / 2; ++div) {
        for (int dot = 1; dot < DIVS_VOLTAGE / 2 * DIVS_SUB; ++dot) {
            float dotPosition = (float)dot / DIVS_SUB;
            gridDrawCounts[0] += 4;
            vaGrid.push_back(QVector3D(-div, -dotPosition, 0));
            vaGrid.push_back(QVector3D(-div, dotPosition, 0));
            vaGrid.push_back(QVector3D(div, -dotPosition, 0));
            vaGrid.push_back(QVector3D(div, dotPosition, 0));
        }
    }
    // Draw horizontal lines
    for (int div = 1; div < DIVS_VOLTAGE / 2; ++div) {
        for (int dot = 1; dot < DIVS_TIME / 2 * DIVS_SUB; ++dot) {
            if (dot % DIVS_SUB == 0) continue; // Already done by vertical lines
            float dotPosition = (float)dot / DIVS_SUB;
            gridDrawCounts[0] += 4;
            vaGrid.push_back(QVector3D(-dotPosition, -div, 0));
            vaGrid.push_back(QVector3D(dotPosition, -div, 0));
            vaGrid.push_back(QVector3D(-dotPosition, div, 0));
            vaGrid.push_back(QVector3D(dotPosition, div, 0));
        }
    }

    { // Bind draw axes
        m_vaoGrid[1].create();
        QOpenGLVertexArrayObject::Binder b(&m_vaoGrid[1]);
        m_grid.bind();
        m_program->enableAttributeArray(vertexLocation);
        m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, int(vaGrid.size() * sizeof(QVector3D)), 3);
    }

    // Axes
    // Horizontal axis
    gridDrawCounts[1] += 4;
    vaGrid.push_back(QVector3D(-DIVS_TIME / 2, 0, 0));
    vaGrid.push_back(QVector3D(DIVS_TIME / 2, 0, 0));
    // Vertical axis
    vaGrid.push_back(QVector3D(0, -DIVS_VOLTAGE / 2, 0));
    vaGrid.push_back(QVector3D(0, DIVS_VOLTAGE / 2, 0));
    // Subdiv lines on horizontal axis
    for (int line = 1; line < DIVS_TIME / 2 * DIVS_SUB; ++line) {
        float linePosition = (float)line / DIVS_SUB;
        gridDrawCounts[1] += 4;
        vaGrid.push_back(QVector3D(linePosition, -0.05f, 0));
        vaGrid.push_back(QVector3D(linePosition, 0.05f, 0));
        vaGrid.push_back(QVector3D(-linePosition, -0.05f, 0));
        vaGrid.push_back(QVector3D(-linePosition, 0.05f, 0));
    }
    // Subdiv lines on vertical axis
    for (int line = 1; line < DIVS_VOLTAGE / 2 * DIVS_SUB; ++line) {
        float linePosition = (float)line / DIVS_SUB;
        gridDrawCounts[1] += 4;
        vaGrid.push_back(QVector3D(-0.05f, linePosition, 0));
        vaGrid.push_back(QVector3D(0.05f, linePosition, 0));
        vaGrid.push_back(QVector3D(-0.05f, -linePosition, 0));
        vaGrid.push_back(QVector3D(0.05f, -linePosition, 0));
    }

    {
        m_vaoGrid[2].create();
        QOpenGLVertexArrayObject::Binder b(&m_vaoGrid[2]);
        m_grid.bind();
        m_program->enableAttributeArray(vertexLocation);
        m_program->setAttributeBuffer(vertexLocation, GL_FLOAT, int(vaGrid.size() * sizeof(QVector3D)), 3);
    }

    // Border
    gridDrawCounts[2] += 4;
    vaGrid.push_back(QVector3D(-DIVS_TIME / 2, -DIVS_VOLTAGE / 2, 0));
    vaGrid.push_back(QVector3D(DIVS_TIME / 2, -DIVS_VOLTAGE / 2, 0));
    vaGrid.push_back(QVector3D(DIVS_TIME / 2, DIVS_VOLTAGE / 2, 0));
    vaGrid.push_back(QVector3D(-DIVS_TIME / 2, DIVS_VOLTAGE / 2, 0));

    m_grid.allocate(&vaGrid[0], int(vaGrid.size() * sizeof(QVector3D)));
    m_grid.release();
}

void GlScope::drawMarkers() {
    auto *gl = context()->versionFunctions<OPENGL_VER>();
    QColor trColor = view->screen.markers;
    m_program->setUniformValue(colorLocation, trColor);

    m_vaoMarker.bind();

    for (unsigned marker = 0; marker < MARKER_COUNT; ++marker) {
        if (!scope->horizontal.marker_visible[marker]) continue;
        vaMarker[marker] = {QVector3D((GLfloat)scope->horizontal.marker[marker], -DIVS_VOLTAGE, 0.0f),
                            QVector3D((GLfloat)scope->horizontal.marker[marker], DIVS_VOLTAGE, 0.0f)};

        m_marker.bind();
        auto ptr = m_marker.mapRange(0, sizeof(Line), QOpenGLBuffer::RangeInvalidateBuffer | QOpenGLBuffer::RangeWrite);
        memcpy(ptr, &vaMarker[marker], sizeof(Line));
        m_marker.unmap();

        gl->glLineWidth((marker == selectedMarker) ? 3 : 1);
        gl->glDrawArrays(GL_LINES, 0, (GLsizei)2);
    }

    m_vaoMarker.release();
}

void GlScope::drawVoltageChannelGraph(ChannelID channel, Graph &graph, int historyIndex) {
    if (!scope->voltage[channel].used) return;

    m_program->setUniformValue(colorLocation, view->screen.voltage[channel].darker(100 + 10 * historyIndex));

    Graph::VaoCount &v = graph.vaoVoltage[channel];

    QOpenGLVertexArrayObject::Binder b(v.first);
    const GLenum dMode = (view->interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP;
    context()->versionFunctions<OPENGL_VER>()->glDrawArrays(dMode, 0, v.second);
}

void GlScope::drawSpectrumChannelGraph(ChannelID channel, Graph &graph, int historyIndex) {
    if (!scope->spectrum[channel].used) return;

    m_program->setUniformValue(colorLocation, view->screen.spectrum[channel].darker(100 + 10 * historyIndex));
    Graph::VaoCount &v = graph.vaoSpectrum[channel];

    QOpenGLVertexArrayObject::Binder b(v.first);
    const GLenum dMode = (view->interpolation == Dso::INTERPOLATION_OFF) ? GL_POINTS : GL_LINE_STRIP;
    context()->versionFunctions<OPENGL_VER>()->glDrawArrays(dMode, 0, v.second);
}
