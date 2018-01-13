// SPDX-License-Identifier: GPL-2.0+

#pragma once

#include <memory>
#include <list>

#include <QtGlobal>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>

#include "glscopegraph.h"
#include "hantekdso/enums.h"
#include "hantekprotocol/types.h"

struct DsoSettingsView;
struct DsoSettingsScope;
class PPresult;

/// \brief OpenGL accelerated widget that displays the oscilloscope screen.
class GlScope : public QOpenGLWidget {
    Q_OBJECT

  public:
    static GlScope *createNormal(DsoSettingsScope *scope, DsoSettingsView *view,
                                 QWidget *parent = 0);
    static GlScope *createZoomed(DsoSettingsScope *scope, DsoSettingsView *view,
                                 QWidget *parent = 0);

    /**
     * We need at least OpenGL 3.2 with shader version 150 or
     * OpenGL ES 2.0 with shader version 100.
     */
    static void fixOpenGLversion();
    /**
     * Show new post processed data
     * @param data
     */
    void showData(PPresult* data);
    void markerUpdated();

  protected:
    /// \brief Initializes the scope widget.
    /// \param settings The settings that should be used.
    /// \param parent The parent widget.
    GlScope(DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent = 0);
    virtual ~GlScope();
    GlScope(const GlScope&) = delete;

    /// \brief Initializes OpenGL output.
    virtual void initializeGL() override;

    /// \brief Draw the graphs, marker and the grid.
    virtual void paintGL() override;

    /// \brief Resize the widget.
    /// \param width The new width of the widget.
    /// \param height The new height of the widget.
    virtual void resizeGL(int width, int height) override;

    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void paintEvent(QPaintEvent *event) override;

    /// \brief Draw the grid.
    void drawGrid();
    /// Draw vertical lines at marker positions
    void drawMarkers();

    void drawVoltageChannelGraph(ChannelID channel, Graph &graph, int historyIndex);
    void drawSpectrumChannelGraph(ChannelID channel, Graph &graph, int historyIndex);
  signals:
    void markerMoved(unsigned marker, double position);

  private:
    // User settings
    DsoSettingsScope *scope;
    DsoSettingsView *view;
    bool zoomed = false;

    // Marker
    const unsigned NO_MARKER = UINT_MAX;
    #pragma pack(push, 1)
    struct Line {
        QVector3D x;
        QVector3D y;
    };
    #pragma pack(pop)
    std::vector<Line> vaMarker;
    unsigned selectedMarker = NO_MARKER;
    QOpenGLBuffer m_marker;
    QOpenGLVertexArrayObject m_vaoMarker;

    // Grid
    QOpenGLBuffer m_grid;
    QOpenGLVertexArrayObject m_vaoGrid[3];
    GLsizei gridDrawCounts[3];
    void generateGrid(QOpenGLShaderProgram *program);

    // Graphs
    std::list<Graph> m_GraphHistory;
    unsigned currentGraphInHistory = 0;

    // OpenGL shader, matrix, var-locations
    bool shaderCompileSuccess = false;
    QString errorMessage;
    std::unique_ptr<QOpenGLShaderProgram> m_program;
    QMatrix4x4 pmvMatrix; ///< projection, view matrix
    int colorLocation;
    int vertexLocation;
    int matrixLocation;
    int selectionLocation;
};
