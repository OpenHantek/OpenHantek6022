// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QtGlobal>

#include "post/ppresult.h"

struct Graph {
    explicit Graph();
    Graph( const Graph & ) = delete;
    Graph( const Graph && ) = delete;
    ~Graph();
    void writeData( PPresult *data, QOpenGLShaderProgram *program, int vertexLocation );
    typedef std::pair< QOpenGLVertexArrayObject *, GLsizei > VaoCount;

  public:
    int allocatedMem = 0;
    QOpenGLBuffer buffer;
    std::vector< VaoCount > vaoVoltage;
    std::vector< VaoCount > vaoHistogram;
    std::vector< VaoCount > vaoSpectrum;
};
