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
struct DsoSettingsScopeCursor;
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
    static void fixOpenGLversion(QSurfaceFormat::RenderableType t=QSurfaceFormat::DefaultRenderableType);
    /**
     * Show new post processed data
     * @param data
     */
    void showData(std::shared_ptr<PPresult> data);
    void updateCursor(unsigned index = 0);
    void cursorSelected(unsigned index) { selectedCursor = index; updateCursor(index); }

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
    void generateVertices(unsigned marker, const DsoSettingsScopeCursor &cursor);
    void drawVertices(QOpenGLFunctions *gl, unsigned marker, QColor color);

    void drawVoltageChannelGraph(ChannelID channel, Graph &graph, int historyIndex);
    void drawSpectrumChannelGraph(ChannelID channel, Graph &graph, int historyIndex);
    QPointF eventToPosition(QMouseEvent *event);
  signals:
    void markerMoved(unsigned cursorIndex, unsigned marker);

  private:
    // User settings
    DsoSettingsScope *scope;
    DsoSettingsView *view;
    bool zoomed = false;

    // Marker
    const unsigned NO_MARKER = UINT_MAX;
    #pragma pack(push, 1)
    struct Vertices {
        QVector3D a, b, c, d;
    };
    #pragma pack(pop)
    const unsigned VERTICES_ARRAY_SIZE = sizeof(Vertices) / sizeof(QVector3D);
    std::vector<Vertices> vaMarker;
    unsigned selectedMarker = NO_MARKER;
    QOpenGLBuffer m_marker;
    QOpenGLVertexArrayObject m_vaoMarker;

    // Cursors
    std::vector<DsoSettingsScopeCursor *> cursorInfo;
    unsigned selectedCursor = 0;

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
