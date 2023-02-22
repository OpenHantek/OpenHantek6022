// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <iostream>

#include <QColor>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QPainter>

#include <QOffscreenSurface>
#include <QOpenGLFunctions>

#include "glscope.h"

#include "post/graphgenerator.h"
#include "post/ppresult.h"
#include "scopesettings.h"
#include "viewconstants.h"
#include "viewsettings.h"


// static info strings
QString GlScope::OpenGLversion;
QString GlScope::GLSLversion;


QString GlScope::getOpenGLversion() {
    if ( OpenGLversion.isNull() ) {
        QOffscreenSurface surface;
        surface.create();
        QOpenGLContext context;
        context.create();
        context.makeCurrent( &surface );
        OpenGLversion = reinterpret_cast< const char * >( context.functions()->glGetString( GL_VERSION ) );
        // qDebug() << OpenGLversion;
        surface.destroy();
    }
    return OpenGLversion;
}


// this static function will be called early from main to set up OpenGL
void GlScope::useOpenGLSLversion( QString renderer ) {
    // QCoreApplication::setAttribute( Qt::AA_ShareOpenGLContexts, true ); // commented out because too late
    QSurfaceFormat format;
    GLSLversion = renderer;
    format.setSamples( 4 ); // ignore antialiasing warning with some HW, Qt & OpenGL versions.
    format.setProfile( QSurfaceFormat::CoreProfile );
    if ( renderer == GLES100 ) {
        format.setRenderableType( QSurfaceFormat::OpenGLES );
        // QCoreApplication::setAttribute( Qt::AA_UseOpenGLES, true ); // commented out because too late
    } else {
        format.setRenderableType( QSurfaceFormat::OpenGL );
        // QCoreApplication::setAttribute( Qt::AA_UseOpenGLES, false ); // commented out because too late
    }
    QSurfaceFormat::setDefaultFormat( format );
}


GlScope::GlScope( DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent )
    : QOpenGLWidget( parent ), scope( scope ), view( view ) {
    if ( scope->verboseLevel > 1 )
        qDebug() << " GLScope::GLScope()";
    cursorInfo.clear();
    cursorInfo.push_back( &scope->horizontal.cursor );
    selectedCursor = 0;
    for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
        cursorInfo.push_back( &scope->voltage[ channel ].cursor );
    }
    for ( ChannelID channel = 0; channel < scope->spectrum.size(); ++channel ) {
        cursorInfo.push_back( &scope->spectrum[ channel ].cursor );
    }
    vaMarker.resize( cursorInfo.size() );
}


GlScope::~GlScope() { // virtual destructor necessary
    if ( scope->verboseLevel > 1 )
        qDebug() << " GLScope::~GLScope()";
}


// static
GlScope *GlScope::createNormal( DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent ) {
    GlScope *s = new GlScope( scope, view, parent );
    s->zoomed = false;
    return s;
}


// static
GlScope *GlScope::createZoomed( DsoSettingsScope *scope, DsoSettingsView *view, QWidget *parent ) {
    GlScope *s = new GlScope( scope, view, parent );
    s->zoomed = true;
    return s;
}


void GlScope::setVisible( bool visible ) {
    if ( !visible && rightMouseInside ) { // clean up the display
        QGuiApplication::restoreOverrideCursor();
        emit cursorMeasurement();
    }
    rightMouseInside = false;
    QWidget::setVisible( visible ); // ... and call the base class method
}


// convert widget position ([0..width()], [0..height()]) to scope-div values (x=[-5..5], y=[4..-4])
QPointF GlScope::posToScopePos( QPointF pos ) {
    QPointF position( ( pos.x() - double( width() ) / 2.0 ) * DIVS_TIME / double( width() ),
                      ( double( height() ) / 2.0 - pos.y() ) * DIVS_VOLTAGE / double( height() ) );
    if ( zoomed ) {
        double m1 = scope->getMarker( 0 );
        double m2 = scope->getMarker( 1 );
        if ( m1 > m2 )
            std::swap( m1, m2 );
        position.setX( m1 + ( 0.5 + ( position.x() / DIVS_TIME ) ) * ( m2 - m1 ) );
    }
    return position;
}


void GlScope::rightMouseEvent( QMouseEvent *event ) {
    if ( rect().contains( event->pos() ) ) {
        if ( !rightMouseInside ) {                                            // enter scope frame
            QGuiApplication::setOverrideCursor( QCursor( Qt::CrossCursor ) ); // switch to measure cursor
        }
        rightMouseInside = true;
        emit cursorMeasurement( posToScopePos( event->pos() ), event->globalPos(), true );
    } else {
        if ( rightMouseInside ) {                     // leave scope frame
            QGuiApplication::restoreOverrideCursor(); // back to normal cursor
        }
        rightMouseInside = false;
        emit cursorMeasurement();
    }
}


void GlScope::mousePressEvent( QMouseEvent *event ) {
    QPointF position = posToScopePos( event->pos() );
    if ( scope->verboseLevel > 3 )
        qDebug() << "   GLS::mPE()" << event;
    if ( !( zoomed && selectedCursor == 0 ) && event->button() == Qt::LeftButton ) {
        selectedMarker = NO_MARKER;
        DsoSettingsScopeCursor *cursor = cursorInfo[ size_t( selectedCursor ) ];
        // Capture nearest marker located within snap area (+/- 1% of full scale).
        double dX0 = fabs( cursor->pos[ 0 ].x() - position.x() );
        double dX1 = fabs( cursor->pos[ 1 ].x() - position.x() );
        double dY0 = fabs( cursor->pos[ 0 ].y() - position.y() );
        double dY1 = fabs( cursor->pos[ 1 ].y() - position.y() );

        switch ( cursor->shape ) {
        case DsoSettingsScopeCursor::RECTANGULAR:
            if ( std::min( dX0, dX1 ) < 1.0 / DIVS_SUB && std::min( dY0, dY1 ) < 1.0 / DIVS_SUB ) {
                // Do we need to swap Y-coords?
                if ( ( dX0 < dX1 && dY0 > dY1 ) || ( dX0 > dX1 && dY0 < dY1 ) ) {
                    std::swap( cursor->pos[ 0 ].ry(), cursor->pos[ 1 ].ry() );
                }
                selectedMarker = ( dX0 < dX1 ) ? 0 : 1;
            }
            break;
        case DsoSettingsScopeCursor::VERTICAL:
            if ( dX0 < dX1 ) {
                if ( dX0 < 1.0 / DIVS_SUB )
                    selectedMarker = 0;
            } else {
                if ( dX1 < 1.0 / DIVS_SUB )
                    selectedMarker = 1;
            }
            break;
        case DsoSettingsScopeCursor::HORIZONTAL:
            if ( dY0 < dY1 ) {
                if ( dY0 < 1.0 / DIVS_SUB )
                    selectedMarker = 0;
            } else {
                if ( dY1 < 1.0 / DIVS_SUB )
                    selectedMarker = 1;
            }
            break;
        case DsoSettingsScopeCursor::NONE:
            break;
        }
        if ( selectedMarker != NO_MARKER ) {
            cursorInfo[ size_t( selectedCursor ) ]->pos[ size_t( selectedMarker ) ] = position;
            if ( selectedCursor == 0 )
                emit markerMoved( selectedCursor, selectedMarker );
        }
    } else if ( ( event->buttons() & Qt::RightButton ) )
        rightMouseEvent( event );
    event->accept();
}


void GlScope::mouseMoveEvent( QMouseEvent *event ) {
    QPointF position = posToScopePos( event->pos() );
    if ( scope->verboseLevel > 3 )
        qDebug() << "   GLS::mME()" << event << position;
    if ( !( zoomed && selectedCursor == 0 ) && ( event->buttons() & Qt::LeftButton ) != 0 ) {
        if ( selectedMarker == NO_MARKER ) {
            // qDebug() << "mouseMoveEvent";
            // User started dragging outside the snap area of any marker:
            // move all markers to current position and select last marker in the array.
            for ( int marker = 0; marker < 2; ++marker ) {
                cursorInfo[ size_t( selectedCursor ) ]->pos[ marker ] = position;
                emit markerMoved( selectedCursor, marker );
                selectedMarker = marker;
            }
        } else if ( selectedMarker < 2 ) {
            cursorInfo[ size_t( selectedCursor ) ]->pos[ selectedMarker ] = position;
            emit markerMoved( selectedCursor, selectedMarker );
        }
    } else if ( event->buttons() & Qt::RightButton )
        rightMouseEvent( event );
    event->accept();
}


void GlScope::mouseReleaseEvent( QMouseEvent *event ) {
    if ( scope->verboseLevel > 3 )
        qDebug() << "   GLS::mRE()" << event;
    if ( !( zoomed && selectedCursor == 0 ) && event->button() == Qt::LeftButton ) {
        QPointF position = posToScopePos( event->pos() );
        if ( selectedMarker < 2 ) {
            // qDebug() << "mouseReleaseEvent";
            cursorInfo[ size_t( selectedCursor ) ]->pos[ selectedMarker ] = position;
            emit markerMoved( selectedCursor, selectedMarker );
        }
        selectedMarker = NO_MARKER;
    }
    if ( rightMouseInside ) {
        QGuiApplication::restoreOverrideCursor();
        emit cursorMeasurement();
    }
    rightMouseInside = false;
    event->accept();
}


void GlScope::mouseDoubleClickEvent( QMouseEvent *event ) {
    if ( scope->verboseLevel > 3 )
        qDebug() << "   GLS::mDCE()" << event;
    if ( !( zoomed && selectedCursor == 0 ) && ( event->buttons() & Qt::LeftButton ) != 0 ) {
        // left double click positions two markers left and right of clicked pos with zoom=100
        QPointF position = posToScopePos( event->pos() );
        if ( selectedMarker == NO_MARKER ) {
            // User double clicked outside the snap area of any marker
            QPointF p = QPointF( 0.5, 0 );       // 10x zoom
            if ( event->modifiers() & Qt::CTRL ) // 100x zoom
                p /= 10;
            if ( event->modifiers() & Qt::SHIFT ) // center at trigger position
                position = QPointF( 10 * scope->trigger.position - 5, 0 );
            // move 1st marker left of current position.
            cursorInfo[ size_t( selectedCursor ) ]->pos[ 0 ] = position - p;
            emit markerMoved( selectedCursor, 0 );
            // move 2nd marker right of current position to make zoom=10 or 100.
            cursorInfo[ size_t( selectedCursor ) ]->pos[ 1 ] = position + p;
            emit markerMoved( selectedCursor, 1 );
            //  select no marker
            selectedMarker = NO_MARKER;
        }
    } else if ( !( zoomed && selectedCursor == 0 ) && ( event->buttons() & Qt::RightButton ) != 0 ) {
        // right double click moves all markers out of the way
        cursorInfo[ size_t( selectedCursor ) ]->pos[ 0 ] = QPointF( MARGIN_LEFT, 0 );
        cursorInfo[ size_t( selectedCursor ) ]->pos[ 1 ] = QPointF( MARGIN_RIGHT, 0 );
        emit markerMoved( selectedCursor, 0 );
        emit markerMoved( selectedCursor, 1 );
    }
    event->accept();
}


void GlScope::wheelEvent( QWheelEvent *event ) {
    if ( scope->verboseLevel > 3 )
        qDebug() << "   GLS::wE()" << event;
    // qDebug() << "wheeelEvent" << zoomed << selectedCursor << event->globalPosition();
    if ( selectedMarker == NO_MARKER ) {
        double step = event->angleDelta().y() / 1200.0; // one click = 0.1
        double &m1 = cursorInfo[ size_t( selectedCursor ) ]->pos[ 0 ].rx();
        double &m2 = cursorInfo[ size_t( selectedCursor ) ]->pos[ 1 ].rx();
        if ( m1 > m2 )
            std::swap( m1, m2 );
        double dm = m2 - m1;
        if ( event->modifiers() & Qt::CTRL ) {                           // zoom in (step > 0) / out (step < 0)
            if ( ( step > 0 && dm <= 1 ) || ( step < 0 && dm <= 0.99 ) ) // smaller steps when zoom >= 10x
                step *= 0.1;
            if ( step < 0 || dm >= 5 * step ) { // step in and new zomm will be < 250x
                m1 += step;
                m2 -= step;
            } else { // set highest zoom  -> 500x fix
                double mm = ( m1 + m2 ) / 2;
                m1 = mm - 0.01;
                m2 = mm + 0.01;
            }
        } else {                                        // move zoom window left/right (100 steps on original)
            if ( step < 0 )                             // shift zoom range left ..
                step = qMax( step, MARGIN_LEFT - m1 );  // .. until m1 == MARGIN_LEFT
            else                                        // shift zoom range right ..
                step = qMin( step, MARGIN_RIGHT - m2 ); // .. until m2 == MARGIN_RIGHT
            if ( event->modifiers() & Qt::SHIFT ) {     // shift -> smaller steps
                if ( m2 - m1 < .1 )                     // zoom > 100 ?
                    step *= ( m2 - m1 );                // .. keep 10 steps per zoomed window
                else                                    // otherwise
                    step *= 0.1;                        // .. 1000 steps on original window
            }
            m1 += step;
            m2 += step;
        }

        emit markerMoved( selectedCursor, 0 );
        emit markerMoved( selectedCursor, 1 );
        selectedMarker = NO_MARKER;
    }
    event->accept();
}


void GlScope::paintEvent( QPaintEvent *event ) {
    if ( shaderCompileSuccess ) {
        QOpenGLWidget::paintEvent( event );
    } else if ( !zoomed ) { // draw error message on normal view if OpenGL failed
        QPainter painter( this );
        painter.setRenderHint( QPainter::Antialiasing, true );
        QFont font = painter.font();
        font.setPointSize( 18 );
        painter.setFont( font );
        painter.drawText( rect(), Qt::AlignCenter | Qt::TextWordWrap, errorMessage );
        fprintf( stderr, "%s", errorMessage.toUtf8().data() );
    }
    event->accept(); // consume the event
}


void GlScope::initializeGL() {
    if ( scope->verboseLevel )
        qDebug() << "GLScope::initializeGL()";
    if ( !QOpenGLShaderProgram::hasOpenGLShaderPrograms( context() ) ) {
        errorMessage = tr( "System does not support OpenGL Shading Language (GLSL)" );
        return;
    }
    if ( m_program ) {
        qWarning() << tr( "OpenGL init called twice!" );
        return;
    }

    auto program = std::unique_ptr< QOpenGLShaderProgram >( new QOpenGLShaderProgram( context() ) );

    const char *vertexShaderGL100ES = R"(
          #version 100
          attribute highp vec3 vertex;
          uniform mat4 matrix;
          void main()
          {
              gl_Position = matrix * vec4(vertex, 1.0);
              gl_PointSize = 1.0;
          }
    )";

    const char *vertexShaderGLSL120 = R"(
          #version 120
          attribute highp vec3 vertex;
          uniform mat4 matrix;
          void main()
          {
              gl_Position = matrix * vec4(vertex, 1.0);
              gl_PointSize = 1.0;
          }
    )";

    const char *vertexShaderGLSL150 = R"(
          #version 150
          in highp vec3 vertex;
          uniform mat4 matrix;
          void main()
          {
              gl_Position = matrix * vec4(vertex, 1.0);
              gl_PointSize = 1.0;
          }
    )";

    const char *fragmentShaderGL100ES = R"(
          #version 100
          uniform highp vec4 color;
          void main() { gl_FragColor = color; }
    )";

    const char *fragmentShaderGLSL120 = R"(
          #version 120
          uniform highp vec4 color;
          void main() { gl_FragColor = color; }
    )";

    const char *fragmentShaderGLSL150 = R"(
          #version 150
          uniform highp vec4 color;
          out vec4 flatColor;
          void main() { flatColor = color; }
    )";

    // Compile vertex and fragment shader
    QString GLEShint;
    if ( GLES100 != GLSLversion )                                 // regular OpenGL
        GLEShint = tr( "Try command line option '--useGLES'\n" ); // offer OpenGL ES as fall back solution
    QString OpenGLinfo = "Graphic: " + OpenGLversion;
    renderInfo = OpenGLinfo + " - GLSL version " + GLSLversion;
    if ( !zoomed )
        qDebug() << renderInfo.toLocal8Bit().data();
    if ( GLSL150 == GLSLversion ) { // use version 150 if supported by OpenGL version >= 3.2
        if ( !program->addShaderFromSourceCode( QOpenGLShader::Vertex, vertexShaderGLSL150 ) ||
             !program->addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentShaderGLSL150 ) ) {
            qWarning() << "Switching to GLSL version 1.20, use the command line option '--useGLSL120' or '--useGLES'";
            GLSLversion = GLSL120; // in case of error try version 120 as fall back
        }
    }
    if ( GLSL120 == GLSLversion ) { // this version is supported by OpenGL version >= 2.1 (older HW/SW)
        if ( !program->addShaderFromSourceCode( QOpenGLShader::Vertex, vertexShaderGLSL120 ) ||
             !program->addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentShaderGLSL120 ) ) {
            errorMessage =
                tr( "Failed to compile OpenGL shader programs.\n" ) + GLEShint + OpenGLinfo + QString( "\n" ) + program->log();
            return; // in case of error propose the use of OpenGLES (OpenGL for embedded systems) and stop
        }
    }
    if ( GLES100 == GLSLversion ) { // use OpenGLES
        if ( !program->addShaderFromSourceCode( QOpenGLShader::Vertex, vertexShaderGL100ES ) ||
             !program->addShaderFromSourceCode( QOpenGLShader::Fragment, fragmentShaderGL100ES ) ) {
            errorMessage = tr( "Failed to compile OpenGL shader programs.\n" ) + OpenGLinfo + QString( "\n" ) + program->log();
            return;
        }
    }

    // Link shader pipeline
    if ( !program->link() || !program->bind() ) {
        errorMessage =
            tr( "Failed to link/bind OpenGL shader programs.\n" ) + GLEShint + renderInfo + QString( "\n" ) + program->log();
        return;
    }

    vertexLocation = program->attributeLocation( "vertex" );
    matrixLocation = program->uniformLocation( "matrix" );
    colorLocation = program->uniformLocation( "color" );

    if ( vertexLocation == -1 || colorLocation == -1 || matrixLocation == -1 ) {
        qWarning() << tr( "Failed to locate shader variable." );
        return;
    }

    program->bind();

    auto *gl = context()->functions();
    gl->glDisable( GL_DEPTH_TEST );
    gl->glEnable( GL_BLEND );
    // Enable depth buffer
    gl->glEnable( GL_DEPTH_TEST );

    // Enable back face culling
    gl->glEnable( GL_CULL_FACE );
    gl->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    QColor bg = view->colors->background;
    gl->glClearColor( GLfloat( bg.redF() ), GLfloat( bg.greenF() ), GLfloat( bg.blueF() ), GLfloat( bg.alphaF() ) );

    m_vaoMarker.create();
    QOpenGLVertexArrayObject::Binder b( &m_vaoMarker );
    m_marker.create();
    m_marker.bind();
    m_marker.setUsagePattern( QOpenGLBuffer::StaticDraw );
    m_marker.allocate( int( vaMarker.size() * sizeof( Vertices ) ) );
    program->enableAttributeArray( vertexLocation );
    program->setAttributeBuffer( vertexLocation, GL_FLOAT, 0, 3, 0 );

    updateCursor();

    m_program = std::move( program );

    generateGrid(); // initialize the grid draw structures

    shaderCompileSuccess = true;
}


void GlScope::showData( std::shared_ptr< PPresult > newData ) {
    if ( !shaderCompileSuccess )
        return;
    makeCurrent();
    // Remove too much entries
    while ( view->digitalPhosphorDraws() < m_GraphHistory.size() )
        m_GraphHistory.pop_back();

    // Add if missing
    if ( view->digitalPhosphorDraws() > m_GraphHistory.size() ) {
        m_GraphHistory.resize( m_GraphHistory.size() + 1 );
    }

    // Move last item to front
    m_GraphHistory.splice( m_GraphHistory.begin(), m_GraphHistory, std::prev( m_GraphHistory.end() ) );

    // Add new entry
    m_GraphHistory.front().writeData( newData.get(), m_program.get(), vertexLocation );
    // doneCurrent();

    update();
}


void GlScope::generateVertices( int marker, const DsoSettingsScopeCursor &cursor ) {
    const float Z_ORDER = 1.0f;
    switch ( cursor.shape ) {
    case DsoSettingsScopeCursor::NONE:
        vaMarker[ size_t( marker ) ] = {
            QVector3D( -DIVS_TIME, -DIVS_VOLTAGE, Z_ORDER ), QVector3D( -DIVS_TIME, DIVS_VOLTAGE, Z_ORDER ),
            QVector3D( DIVS_TIME, DIVS_VOLTAGE, Z_ORDER ), QVector3D( DIVS_TIME, -DIVS_VOLTAGE, Z_ORDER ) };
        break;
    case DsoSettingsScopeCursor::VERTICAL:
        vaMarker[ size_t( marker ) ] = { QVector3D( GLfloat( cursor.pos[ 0 ].x() ), -GLfloat( DIVS_VOLTAGE ), Z_ORDER ),
                                         QVector3D( GLfloat( cursor.pos[ 0 ].x() ), GLfloat( DIVS_VOLTAGE ), Z_ORDER ),
                                         QVector3D( GLfloat( cursor.pos[ 1 ].x() ), GLfloat( DIVS_VOLTAGE ), Z_ORDER ),
                                         QVector3D( GLfloat( cursor.pos[ 1 ].x() ), -GLfloat( DIVS_VOLTAGE ), Z_ORDER ) };
        break;
    case DsoSettingsScopeCursor::HORIZONTAL:
        vaMarker[ size_t( marker ) ] = { QVector3D( -GLfloat( DIVS_TIME ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ),
                                         QVector3D( GLfloat( DIVS_TIME ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ),
                                         QVector3D( GLfloat( DIVS_TIME ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ),
                                         QVector3D( -GLfloat( DIVS_TIME ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ) };
        break;
    case DsoSettingsScopeCursor::RECTANGULAR:
        if ( ( cursor.pos[ 1 ].x() - cursor.pos[ 0 ].x() ) * ( cursor.pos[ 1 ].y() - cursor.pos[ 0 ].y() ) > 0.0 ) {
            vaMarker[ size_t( marker ) ] = { QVector3D( GLfloat( cursor.pos[ 0 ].x() ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 1 ].x() ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 1 ].x() ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 0 ].x() ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ) };
        } else {
            vaMarker[ size_t( marker ) ] = { QVector3D( GLfloat( cursor.pos[ 0 ].x() ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 0 ].x() ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 1 ].x() ), GLfloat( cursor.pos[ 1 ].y() ), Z_ORDER ),
                                             QVector3D( GLfloat( cursor.pos[ 1 ].x() ), GLfloat( cursor.pos[ 0 ].y() ), Z_ORDER ) };
        }
        break;
    }
}


void GlScope::selectCursor( int index ) {
    selectedCursor = index;
    updateCursor( index );
}


void GlScope::updateCursor( int index ) {
    if ( index > 0 ) {
        generateVertices( index, *cursorInfo[ size_t( index ) ] );
    } else
        for ( index = 0; index < int( cursorInfo.size() ); ++index ) {
            generateVertices( index, *cursorInfo[ size_t( index ) ] );
        }
    // Write coordinates to GPU
    makeCurrent();
    m_marker.bind();
    m_marker.write( 0, vaMarker.data(), int( vaMarker.size() * sizeof( Vertices ) ) );
}


void GlScope::paintGL() {
    if ( !shaderCompileSuccess )
        return;

    auto *gl = context()->functions();

    QColor bg = view->colors->background;
    gl->glClearColor( GLfloat( bg.redF() ), GLfloat( bg.greenF() ), GLfloat( bg.blueF() ), GLfloat( bg.alphaF() ) );

    // Clear OpenGL buffer and configure settings
    // TODO Don't clear if view->digitalPhosphorDraws()>1
    gl->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    gl->glLineWidth( 1 );

    m_program->bind();

    // Apply zoom settings via matrix transformation
    if ( zoomed ) {
        QMatrix4x4 m;
        m.scale( QVector3D( GLfloat( DIVS_TIME ) / GLfloat( fabs( scope->getMarker( 1 ) - scope->getMarker( 0 ) ) ), 1.0f, 1.0f ) );
        m.translate( -GLfloat( scope->getMarker( 0 ) + scope->getMarker( 1 ) ) / 2, 0.0f, 0.0f );
        m_program->setUniformValue( matrixLocation, pmvMatrix * m );
    }

    drawMarkers();

    unsigned historyIndex = 0;
    for ( Graph &graph : m_GraphHistory ) {
        for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel ) {
            if ( scope->horizontal.format == Dso::GraphFormat::TY ) {
                drawSpectrumChannelGraph( channel, graph, int( historyIndex ) );
                if ( scope->histogram ) {
                    drawHistogramChannelGraph( channel, graph, int( historyIndex ) );
                }
            }
            drawVoltageChannelGraph( channel, graph, int( historyIndex ) );
        }
        ++historyIndex;
    }

    if ( zoomed ) {
        m_program->setUniformValue( matrixLocation, pmvMatrix );
    }

    drawGrid();
    m_program->release();
}


void GlScope::resizeGL( int width, int height ) {
    if ( !shaderCompileSuccess )
        return;
    auto *gl = context()->functions();
    gl->glViewport( 0, 0, GLint( width ), GLint( height ) );

    // Set axes to div-scale and apply correction for exact pixelization
    float pixelizationWidthCorrection = float( width ) / float( width - 1 );
    float pixelizationHeightCorrection = float( height ) / float( height - 1 );

    pmvMatrix.setToIdentity();
    pmvMatrix.ortho( -float( DIVS_TIME ) / 2.0f * pixelizationWidthCorrection,
                     float( DIVS_TIME ) / 2.0f * pixelizationWidthCorrection,
                     -float( DIVS_VOLTAGE ) / 2.0f * pixelizationHeightCorrection,
                     float( DIVS_VOLTAGE ) / 2.0f * pixelizationHeightCorrection, -1.0f, 1.0f );

    m_program->bind();
    m_program->setUniformValue( matrixLocation, pmvMatrix );
    m_program->release();
}


// draw 4 small crosses @ (x,y), (-x,y), (x,-y) and (-x,-y)
// section 0:grid, 1:axes, 2:border
void GlScope::draw4Cross( std::vector< QVector3D > &va, int section, float x, float y ) {
    const float d = 0.05f; // cross size
    for ( float xSign : { -1.0f, 1.0f } ) {
        for ( float ySign : { -1.0f, 1.0f } ) {
            gridDrawCounts[ section ] += 4;
            va.push_back( QVector3D( xSign * ( x - d ), ySign * y, 0 ) );
            va.push_back( QVector3D( xSign * ( x + d ), ySign * y, 0 ) );
            va.push_back( QVector3D( xSign * x, ySign * ( y - d ), 0 ) );
            va.push_back( QVector3D( xSign * x, ySign * ( y + d ), 0 ) );
        }
    }
}


// prepare the static grid structure that is shown by 'drawGrid()'
// show a line as long as the trigger level marker is clicked or moved
// index: channel, value: trigger level, pressed: marker is activated
void GlScope::generateGrid( int index, double value, bool pressed ) {
    // printf( "prepareGrid( %d, %g, %d )\n", index, value, pressed );

    QOpenGLShaderProgram *program = m_program.get();
    if ( program == nullptr )
        return;

    for ( int iii = 0; iii < gridItems; ++iii )
        gridDrawCounts[ iii ] = 0;

    if ( !m_grid.isCreated() )
        m_grid.create();
    m_grid.bind();
    m_grid.setUsagePattern( QOpenGLBuffer::StaticDraw );

    std::vector< QVector3D > vaGrid;

    int item = 0;

    { // Bind draw vertical lines
        if ( !m_vaoGrid[ item ].isCreated() )
            m_vaoGrid[ item ].create();
        QOpenGLVertexArrayObject::Binder b( &m_vaoGrid[ item ] );
        m_grid.bind();
        program->enableAttributeArray( vertexLocation );
        program->setAttributeBuffer( vertexLocation, GL_FLOAT, 0, 3, 0 );
    }

    // Draw vertical dot lines
    for ( int vDiv = 1; vDiv < DIVS_TIME / 2; ++vDiv ) {
        for ( int dot = 1; dot < DIVS_VOLTAGE / 2 * DIVS_SUB; ++dot ) {
            float dotPosition = float( dot ) / DIVS_SUB;
            gridDrawCounts[ item ] += 4;
            vaGrid.push_back( QVector3D( -float( vDiv ), -dotPosition, 0 ) );
            vaGrid.push_back( QVector3D( -float( vDiv ), dotPosition, 0 ) );
            vaGrid.push_back( QVector3D( float( vDiv ), -dotPosition, 0 ) );
            vaGrid.push_back( QVector3D( float( vDiv ), dotPosition, 0 ) );
        }
    }
    // Draw horizontal dot lines
    for ( int hDiv = 1; hDiv < DIVS_VOLTAGE / 2; ++hDiv ) {
        for ( int dot = 1; dot < DIVS_TIME / 2 * DIVS_SUB; ++dot ) {
            if ( dot % DIVS_SUB == 0 )
                continue; // Already done by vertical lines
            float dotPosition = float( dot ) / DIVS_SUB;
            gridDrawCounts[ item ] += 4;
            vaGrid.push_back( QVector3D( -dotPosition, -float( hDiv ), 0 ) );
            vaGrid.push_back( QVector3D( dotPosition, -float( hDiv ), 0 ) );
            vaGrid.push_back( QVector3D( -dotPosition, float( hDiv ), 0 ) );
            vaGrid.push_back( QVector3D( dotPosition, float( hDiv ), 0 ) );
        }
    }

    ++item;

    { // Bind draw axes
        if ( !m_vaoGrid[ item ].isCreated() )
            m_vaoGrid[ item ].create();
        QOpenGLVertexArrayObject::Binder b( &m_vaoGrid[ item ] );
        m_grid.bind();
        program->enableAttributeArray( vertexLocation );
        program->setAttributeBuffer( vertexLocation, GL_FLOAT, int( vaGrid.size() * sizeof( QVector3D ) ), 3 );
    }

    // Axes
    // Horizontal axis
    gridDrawCounts[ item ] += 2;
    vaGrid.push_back( QVector3D( -DIVS_TIME / 2, 0, 0 ) );
    vaGrid.push_back( QVector3D( DIVS_TIME / 2, 0, 0 ) );
    // Vertical axis
    gridDrawCounts[ item ] += 2;
    vaGrid.push_back( QVector3D( 0, -DIVS_VOLTAGE / 2, 0 ) );
    vaGrid.push_back( QVector3D( 0, DIVS_VOLTAGE / 2, 0 ) );
    // Subdiv lines on horizontal axis
    for ( int line = 1; line < DIVS_TIME / 2 * DIVS_SUB; ++line ) {
        float linePosition = float( line ) / DIVS_SUB;
        gridDrawCounts[ item ] += 4;
        vaGrid.push_back( QVector3D( linePosition, -0.05f, 0 ) );
        vaGrid.push_back( QVector3D( linePosition, 0.05f, 0 ) );
        vaGrid.push_back( QVector3D( -linePosition, -0.05f, 0 ) );
        vaGrid.push_back( QVector3D( -linePosition, 0.05f, 0 ) );
    }
    // Subdiv lines on vertical axis
    for ( int line = 1; line < DIVS_VOLTAGE / 2 * DIVS_SUB; ++line ) {
        float linePosition = float( line ) / DIVS_SUB;
        gridDrawCounts[ item ] += 4;
        vaGrid.push_back( QVector3D( -0.05f, linePosition, 0 ) );
        vaGrid.push_back( QVector3D( 0.05f, linePosition, 0 ) );
        vaGrid.push_back( QVector3D( -0.05f, -linePosition, 0 ) );
        vaGrid.push_back( QVector3D( 0.05f, -linePosition, 0 ) );
    }

    // Draw vertical cross lines
    for ( int vDiv = 1; vDiv < DIVS_TIME / 2; ++vDiv ) {
        for ( int hDiv = 1; hDiv < DIVS_VOLTAGE / 2; ++hDiv ) {
            draw4Cross( vaGrid, 1, float( vDiv ), float( hDiv ) );
        }
    }
    // Draw horizontal cross lines
    for ( int hDiv = 1; hDiv < DIVS_VOLTAGE / 2; ++hDiv ) {
        for ( int vDiv = 1; vDiv < DIVS_TIME / 2; ++vDiv ) {
            if ( vDiv % DIVS_SUB == 0 )
                continue; // Already done by vertical lines
            draw4Cross( vaGrid, 1, float( vDiv ), float( hDiv ) );
        }
    }

    ++item;

    { // Border
        if ( !m_vaoGrid[ item ].isCreated() )
            m_vaoGrid[ item ].create();
        QOpenGLVertexArrayObject::Binder b( &m_vaoGrid[ item ] );
        m_grid.bind();
        program->enableAttributeArray( vertexLocation );
        program->setAttributeBuffer( vertexLocation, GL_FLOAT, int( vaGrid.size() * sizeof( QVector3D ) ), 3 );
    }
    gridDrawCounts[ item ] += 4;
    vaGrid.push_back( QVector3D( -DIVS_TIME / 2, -DIVS_VOLTAGE / 2, 0 ) );
    vaGrid.push_back( QVector3D( DIVS_TIME / 2, -DIVS_VOLTAGE / 2, 0 ) );
    vaGrid.push_back( QVector3D( DIVS_TIME / 2, DIVS_VOLTAGE / 2, 0 ) );
    vaGrid.push_back( QVector3D( -DIVS_TIME / 2, DIVS_VOLTAGE / 2, 0 ) );

    ++item;

    { // prepare (dynamic) trigger level marker line
        if ( !m_vaoGrid[ item ].isCreated() )
            m_vaoGrid[ item ].create();
        QOpenGLVertexArrayObject::Binder b( &m_vaoGrid[ item ] );
        m_grid.bind();
        program->enableAttributeArray( vertexLocation );
        program->setAttributeBuffer( vertexLocation, GL_FLOAT, int( vaGrid.size() * sizeof( QVector3D ) ), 3 );
    }
    if ( pressed && index >= 0 ) {
        triggerLineColor = view->colors->voltage[ unsigned( index ) ];
        if ( index != int( scope->trigger.source ) )
            triggerLineColor = triggerLineColor.darker();
        float yPos = float( ( value / scope->gain( unsigned( index ) ) + scope->voltage[ unsigned( index ) ].offset ) );
        gridDrawCounts[ item ] += 2;
        vaGrid.push_back( QVector3D( -DIVS_TIME / 2, yPos, 0 ) );
        vaGrid.push_back( QVector3D( DIVS_TIME / 2, yPos, 0 ) );
    }

    m_grid.allocate( &vaGrid[ 0 ], int( vaGrid.size() * sizeof( QVector3D ) ) );
    m_grid.release();
}


void GlScope::drawGrid() {
    auto *gl = context()->functions();

    gl->glLineWidth( 1 );

    int item = 3;
    // Trigger level (draw this on top of the other items)
    m_vaoGrid[ item ].bind();
    m_program->setUniformValue( colorLocation, triggerLineColor );
    gl->glDrawArrays( GL_LINES, 0, gridDrawCounts[ item ] );
    m_vaoGrid[ item ].release();

    // Grid
    item = 0;
    m_vaoGrid[ item ].bind();
    m_program->setUniformValue( colorLocation, view->colors->grid );
    gl->glDrawArrays( GL_POINTS, 0, gridDrawCounts[ item ] );
    m_vaoGrid[ item ].release();

    // Axes and div crosses
    ++item;
    m_vaoGrid[ item ].bind();
    m_program->setUniformValue( colorLocation, view->colors->axes );
    gl->glDrawArrays( GL_LINES, 0, gridDrawCounts[ item ] );
    m_vaoGrid[ item ].release();

    // Border
    ++item;
    m_vaoGrid[ item ].bind();
    m_program->setUniformValue( colorLocation, view->colors->border );
    gl->glDrawArrays( GL_LINE_LOOP, 0, gridDrawCounts[ item ] );
    m_vaoGrid[ item ].release();
}


void GlScope::drawVertices( QOpenGLFunctions *gl, int marker, QColor color ) {
    m_program->setUniformValue( colorLocation, ( marker == selectedCursor ) ? color : color.darker() );
    gl->glDrawArrays( GL_LINE_LOOP, GLint( marker * VERTICES_ARRAY_SIZE ), GLint( VERTICES_ARRAY_SIZE ) );
    if ( cursorInfo[ size_t( marker ) ]->shape == DsoSettingsScopeCursor::RECTANGULAR ) {
        color.setAlphaF( 0.5 ); // increase this value if you encounter hardcopy/print artefacts (?)
        m_program->setUniformValue( colorLocation, color.darker() );
        gl->glDrawArrays( GL_TRIANGLE_FAN, GLint( marker * VERTICES_ARRAY_SIZE ), GLint( VERTICES_ARRAY_SIZE ) );
    }
}


void GlScope::drawMarkers() {
    auto *gl = context()->functions();

    m_vaoMarker.bind();

    int marker = 0;
    drawVertices( gl, marker, view->colors->markers );
    ++marker;

    if ( view->cursorsVisible ) {
        gl->glDepthMask( GL_FALSE );
        for ( ChannelID channel = 0; channel < scope->voltage.size(); ++channel, ++marker ) {
            if ( scope->voltage[ channel ].used ) {
                drawVertices( gl, marker, view->colors->voltage[ channel ] );
            }
        }
        for ( ChannelID channel = 0; channel < scope->spectrum.size(); ++channel, ++marker ) {
            if ( scope->spectrum[ channel ].used ) {
                drawVertices( gl, marker, view->colors->spectrum[ channel ] );
            }
        }
        gl->glDepthMask( GL_TRUE );
    }

    m_vaoMarker.release();
}


void GlScope::drawVoltageChannelGraph( ChannelID channel, Graph &graph, int historyIndex ) {
    if ( !scope->voltage[ channel ].used )
        return;

    m_program->setUniformValue( colorLocation, view->colors->voltage[ channel ].darker( 100 + 10 * historyIndex ) );
    Graph::VaoCount &v = graph.vaoVoltage[ channel ];

    QOpenGLVertexArrayObject::Binder b( v.first );
    const GLenum dMode = ( view->interpolation == Dso::INTERPOLATION_OFF ) ? GL_POINTS : GL_LINE_STRIP;
    context()->functions()->glDrawArrays( dMode, 0, v.second );
}


void GlScope::drawHistogramChannelGraph( ChannelID channel, Graph &graph, int historyIndex ) {
    if ( graph.vaoHistogram.empty() || !scope->voltage[ channel ].used )
        return;

    m_program->setUniformValue( colorLocation, view->colors->voltage[ channel ].darker( 100 + 10 * historyIndex ) );
    Graph::VaoCount &h = graph.vaoHistogram[ channel ];

    QOpenGLVertexArrayObject::Binder b( h.first );
    const GLenum dMode = GL_LINES; // display histogram with lines
    context()->functions()->glDrawArrays( dMode, 0, h.second );
}


void GlScope::drawSpectrumChannelGraph( ChannelID channel, Graph &graph, int historyIndex ) {
    if ( !scope->spectrum[ channel ].used )
        return;

    m_program->setUniformValue( colorLocation, view->colors->spectrum[ channel ].darker( 100 + 10 * historyIndex ) );
    Graph::VaoCount &v = graph.vaoSpectrum[ channel ];

    QOpenGLVertexArrayObject::Binder b( v.first );
    const GLenum dMode = ( view->interpolation == Dso::INTERPOLATION_OFF ) ? GL_POINTS : GL_LINE_STRIP;
    context()->functions()->glDrawArrays( dMode, 0, v.second );
}
