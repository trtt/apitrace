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

#pragma once

class TimelineGraphData : public AbstractGraphData
{
    Q_OBJECT

public:
    TimelineGraphData(const std::shared_ptr<TextureBufferData<GLuint>>& dataColor,
                      unsigned colorSize,
                      const std::shared_ptr<TextureBufferData<GLuint>>& dataFilter)
        : AbstractGraphData(dataFilter), m_dataColor(dataColor), m_colorSize(colorSize) {}

    TextureBufferData<GLuint>* dataColor() const { return m_dataColor.get();}

    void acquireResHandle();
    void freeResHandle();

    unsigned colorSize() const { return m_colorSize; }

private:
    std::shared_ptr<TextureBufferData<GLuint>> m_dataColor;
    unsigned m_colorSize;
};



class TimelineGraph;
class TimelineGraphRenderer : public AbstractGraphRenderer
{
public:
    TimelineGraphRenderer(TimelineGraph* item);
    ~TimelineGraphRenderer();

public:
    void render();
    void synchronizeAfter(QQuickFramebufferObject* item);

private:
    enum ProgramType {
        PROGRAM_NOFILTNOCOLOR_FP64,
        PROGRAM_FILTNOCOLOR_FP64,
        PROGRAM_NOFILTCOLOR_FP64,
        PROGRAM_FILTCOLOR_FP64,
        PROGRAM_NOFILTNOCOLOR_FP32,
        PROGRAM_FILTNOCOLOR_FP32,
        PROGRAM_NOFILTCOLOR_FP32,
        PROGRAM_FILTCOLOR_FP32,
        PROGRAM_SIZE
    };

    bool m_GLinit = false;
    bool m_doublePrecision = false;
    QOpenGLShaderProgram* m_program;
    static QOpenGLShaderProgram* m_programs[PROGRAM_SIZE];
    static QGLBuffer m_vertexBuffer;
    static GLuint m_vao;
    static unsigned m_numInstances;

    bool m_coloredCopy;
};


class TimelineGraph : public AbstractGraph
{
    Q_OBJECT
    Q_PROPERTY(bool colored MEMBER m_colored)

    friend class TimelineGraphRenderer;

public:
    TimelineGraph(QQuickItem *parent = 0) : AbstractGraph(parent) {}
    ~TimelineGraph() {}

    QQuickFramebufferObject::Renderer* createRenderer() const;

private:
    bool m_colored = true;
};
