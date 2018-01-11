#include "glscopegraph.h"
#include <QDebug>

Graph::Graph() : buffer(QOpenGLBuffer::VertexBuffer) {
    buffer.create();
    buffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
}

void Graph::writeData(PPresult *data, QOpenGLShaderProgram *program, int vertexLocation) {
    // Determine memory
    int neededMemory = 0;
    for (ChannelGraph &cg : data->vaChannelVoltage) neededMemory += cg.size() * sizeof(QVector3D);
    for (ChannelGraph &cg : data->vaChannelSpectrum) neededMemory += cg.size() * sizeof(QVector3D);

    buffer.bind();

    // Allocate space if necessary
    if (neededMemory > allocatedMem) {
        buffer.allocate(neededMemory);
        allocatedMem = neededMemory;
    }

    qDebug() << data->data(0)->frequency;

    // Write data to buffer
    int offset = 0;
    vaoVoltage.resize(data->vaChannelVoltage.size());
    vaoSpectrum.resize(data->vaChannelSpectrum.size());
    for (ChannelID channel = 0; channel < vaoVoltage.size(); ++channel) {
        VaoCount &v = vaoVoltage[channel];
        VaoCount &s = vaoSpectrum[channel];
        int dataSize;

        // Voltage channel
        if (!v.first) v.first = new QOpenGLVertexArrayObject;
        ChannelGraph &gVoltage = data->vaChannelVoltage[channel];
        v.first->bind();
        dataSize = int(gVoltage.size() * sizeof(QVector3D));
        buffer.write(offset, gVoltage.data(), dataSize);
        program->enableAttributeArray(vertexLocation);
        program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, 0);
        v.first->release();
        v.second = (int)gVoltage.size();
        offset += dataSize;

        // Spectrum channel
        if (!s.first) s.first = new QOpenGLVertexArrayObject;
        ChannelGraph &gSpectrum = data->vaChannelSpectrum[channel];
        s.first->bind();
        dataSize = int(gSpectrum.size() * sizeof(QVector3D));
        buffer.write(offset, gSpectrum.data(), dataSize);
        program->enableAttributeArray(vertexLocation);
        program->setAttributeBuffer(vertexLocation, GL_FLOAT, offset, 3, 0);
        s.first->release();
        s.second = (int)gSpectrum.size();
        offset += dataSize;
    }

    buffer.release();
}

Graph::~Graph() {
    for (auto &vao : vaoVoltage) {
        vao.first->destroy();
        delete vao.first;
    }
    for (auto &vao : vaoSpectrum) {
        vao.first->destroy();
        delete vao.first;
    }
    if (buffer.isCreated()) { buffer.destroy(); }
}
