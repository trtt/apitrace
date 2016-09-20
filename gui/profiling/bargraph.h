/**************************************************************************
 *
 * Copyright 2016 Alexander Trukhin
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#include "abstractgraph.h"

#include <QRunnable>

#pragma once

class BarGraphData : public AbstractGraphData
{
    Q_OBJECT

public:
    BarGraphData(const std::shared_ptr<TextureBufferData<GLfloat>>& dataY,
                 const std::shared_ptr<TextureBufferData<GLuint>>& dataFilter)
        : AbstractGraphData(dataFilter), m_dataY(dataY) {}

    TextureBufferData<GLfloat>* dataY() const { return m_dataY.get();}
    Q_INVOKABLE float eventYValue(int id) const;

    void acquireResHandle();
    void freeResHandle();

private:
    std::shared_ptr<TextureBufferData<GLfloat>> m_dataY;
};


class BarGraph;
class MaxYUpdater : public QObject, public QRunnable
{
    Q_OBJECT

public:
    MaxYUpdater(BarGraph& bg, int first, int last, unsigned numElements)
        : m_graph(bg), m_firstEvent(first), m_lastEvent(last),
          m_numElements(numElements) {}

public:
    void run();

signals:
    void maxYObtained(float maxY);

private:
    BarGraph& m_graph;
    int m_firstEvent, m_lastEvent;
    unsigned m_numElements;
};


class BarGraph;
class BarGraphRenderer : public AbstractGraphRenderer
{
public:
    BarGraphRenderer(BarGraph* item);
    ~BarGraphRenderer();

public:
    void render();
    void synchronizeAfter(QQuickFramebufferObject* item);

private:
    enum ProgramType {
        PROGRAM_NOFILT_FP64,
        PROGRAM_FILT_FP64,
        PROGRAM_NOFILT_FP32,
        PROGRAM_FILT_FP32,
        PROGRAM_SIZE
    };

    bool m_GLinit = false;
    bool m_doublePrecisionAvailable = false;
    QOpenGLShaderProgram* m_program;
    static QOpenGLShaderProgram* m_programs[PROGRAM_SIZE];
    static QGLBuffer m_vertexBuffer;
    static GLuint m_vao;
    static unsigned m_numInstances;

    GLfloat m_maxYCopy;
    bool m_doublePrecisionCopy;
};


class BarGraph : public AbstractGraph
{
    Q_OBJECT
    Q_PROPERTY(float maxY READ maxY WRITE setMaxY NOTIFY maxYChanged)
    Q_PROPERTY(bool doublePrecision MEMBER m_doublePrecision)

    friend class BarGraphRenderer;

public:
    BarGraph(QQuickItem *parent = 0);
    ~BarGraph();

    float maxY() const { return m_maxY; }
    void setMaxY(float maxy) { m_maxY = maxy; emit maxYChanged(); }

    QQuickFramebufferObject::Renderer* createRenderer() const;

    float maxVisibleEvent() const;

signals:
    void maxYChanged();

public slots:
    void forceupdate();
    void updateMaxY();
    void initAxisConnections() const;

private:
    bool m_needsUpdatingMaxY = false;
    bool m_doublePrecision = false;

    GLfloat m_maxY;
};
