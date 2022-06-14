// SPDX-License-Identifier: GPL-2.0-or-later

#include "glscopegraph.h"
#include <QDebug>
#include <stdexcept>

Graph::Graph() : buffer( QOpenGLBuffer::VertexBuffer ) {
    buffer.create();
    buffer.setUsagePattern( QOpenGLBuffer::DynamicDraw );
}

void Graph::writeData( PPresult *data, QOpenGLShaderProgram *program, int vertexLocation ) {
    // Determine memory
    int neededMemory = 0;
    for ( ChannelGraph &cg : data->vaChannelVoltage )
        neededMemory += int( cg.size() * sizeof( QVector3D ) );
    for ( ChannelGraph &cg : data->vaChannelHistogram )
        neededMemory += int( cg.size() * sizeof( QVector3D ) );
    for ( ChannelGraph &cg : data->vaChannelSpectrum )
        neededMemory += int( cg.size() * sizeof( QVector3D ) );

    buffer.bind();
    program->bind();

    // Allocate space if necessary
    if ( neededMemory > allocatedMem ) {
        buffer.allocate( neededMemory );
        allocatedMem = neededMemory;
    }

    // Write data to buffer
    int offset = 0;
    vaoVoltage.resize( data->vaChannelVoltage.size() );
    vaoHistogram.resize( data->vaChannelHistogram.size() );
    vaoSpectrum.resize( data->vaChannelSpectrum.size() );
    for ( ChannelID channel = 0; channel < std::max( std::max( vaoVoltage.size(), vaoHistogram.size() ), vaoSpectrum.size() );
          ++channel ) {
        int dataSize;

        // Voltage channel
        if ( channel < vaoVoltage.size() ) {
            VaoCount &v = vaoVoltage[ channel ];
            if ( !v.first ) {
                v.first = new QOpenGLVertexArrayObject;
                if ( !v.first->create() )
                    throw new std::runtime_error( "QOpenGLVertexArrayObject create failed" );
            }
            ChannelGraph &gVoltage = data->vaChannelVoltage[ channel ];
            v.first->bind();
            dataSize = int( gVoltage.size() * sizeof( QVector3D ) );
            buffer.write( offset, gVoltage.data(), dataSize );
            program->enableAttributeArray( vertexLocation );
            program->setAttributeBuffer( vertexLocation, GL_FLOAT, offset, 3, 0 );
            v.first->release();
            v.second = int( gVoltage.size() );
            offset += dataSize;
        }

        // Histogram channel
        if ( channel < vaoHistogram.size() ) {
            VaoCount &h = vaoHistogram[ channel ];
            if ( !h.first ) {
                h.first = new QOpenGLVertexArrayObject;
                if ( !h.first->create() )
                    throw new std::runtime_error( "QOpenGLVertexArrayObject create failed" );
            }
            ChannelGraph &gHistogram = data->vaChannelHistogram[ channel ];
            h.first->bind();
            dataSize = int( gHistogram.size() * sizeof( QVector3D ) );
            buffer.write( offset, gHistogram.data(), dataSize );
            program->enableAttributeArray( vertexLocation );
            program->setAttributeBuffer( vertexLocation, GL_FLOAT, offset, 3, 0 );
            h.first->release();
            h.second = int( gHistogram.size() );
            offset += dataSize;
        }

        // Spectrum channel
        if ( channel < vaoSpectrum.size() ) {
            VaoCount &s = vaoSpectrum[ channel ];
            if ( !s.first ) {
                s.first = new QOpenGLVertexArrayObject;
                if ( !s.first->create() )
                    throw new std::runtime_error( "QOpenGLVertexArrayObject create failed" );
            }
            ChannelGraph &gSpectrum = data->vaChannelSpectrum[ channel ];
            s.first->bind();
            dataSize = int( gSpectrum.size() * sizeof( QVector3D ) );
            buffer.write( offset, gSpectrum.data(), dataSize );
            program->enableAttributeArray( vertexLocation );
            program->setAttributeBuffer( vertexLocation, GL_FLOAT, offset, 3, 0 );
            s.first->release();
            s.second = int( gSpectrum.size() );
            offset += dataSize;
        }
    }

    buffer.release();
}

Graph::~Graph() {
    for ( auto &vao : vaoVoltage ) {
        vao.first->destroy();
        delete vao.first;
    }
    for ( auto &vao : vaoHistogram ) {
        vao.first->destroy();
        delete vao.first;
    }
    for ( auto &vao : vaoSpectrum ) {
        vao.first->destroy();
        delete vao.first;
    }
    if ( buffer.isCreated() ) {
        buffer.destroy();
    }
}
