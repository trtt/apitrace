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

#include "bargraph.h"

#include <QQuickWindow>
#include <QThreadPool>

#include <cmath>


void BarGraphData::acquireResHandle() {
    if (!m_numHandles) {
        m_dataY->init();
    }
    AbstractGraphData::acquireResHandle();
}

void BarGraphData::freeResHandle() {
    AbstractGraphData::freeResHandle();
    if (!m_numHandles) {
        if (m_dataY.unique())
            m_dataY->deinit();
    }
}

float BarGraphData::eventYValue(int id) const {
     return m_dataY->data[id];
}


void MaxYUpdater::run() {
    const std::vector<GLfloat>& v = static_cast<BarGraphData*>(m_graph.data())->dataY()->data;

    double stride = (m_lastEvent - m_firstEvent) / (double)(m_numElements-1);
    if (m_firstEvent == m_lastEvent) {
        emit maxYObtained((!m_graph.isEventFiltered(m_lastEvent)) ? v[m_lastEvent] : 0);
    } else if (m_firstEvent > m_lastEvent) {
        emit maxYObtained(0);
    } else {

        GLfloat largest = (!m_graph.isEventFiltered(m_firstEvent)) ? v[m_firstEvent] : 0;
        for (uint i=1; i < m_numElements; ++i) {
            int index = round(m_firstEvent + i * stride);
            if (m_graph.isEventFiltered(index)) continue;
            float tested = v[index];
            if (largest < tested) {
                largest = tested;
            }
        }
        emit maxYObtained(largest);
    }
}


BarGraph::BarGraph(QQuickItem *parent)
    : AbstractGraph(parent), m_maxY(1.f)
{
    connect(this, &BarGraph::maxYChanged, this, &AbstractGraph::forceupdate);
    connect(this, &AbstractGraph::numElementsChanged, this, &BarGraph::updateMaxY);
    connect(this, &AbstractGraph::axisChanged, this, &BarGraph::initAxisConnections);
    auto numThreads = QThreadPool::globalInstance()->maxThreadCount();
    if (numThreads != QThread::idealThreadCount() - 2) {
        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount() - 2);
    }
}

BarGraph::~BarGraph() {
    QThreadPool::globalInstance()->waitForDone();
}

void BarGraph::initAxisConnections() const {
    connect(m_axis, &TimelineAxis::dispStartTimeChanged, this, &BarGraph::updateMaxY);
    connect(m_axis, &TimelineAxis::dispEndTimeChanged, this, &BarGraph::updateMaxY);
}

QQuickFramebufferObject::Renderer* BarGraph::createRenderer() const {
    return new BarGraphRenderer(const_cast<BarGraph*>(this));
}

void BarGraph::forceupdate() {
    AbstractGraph::forceupdate();
}

float BarGraph::maxVisibleEvent() const {
    int firstEvent = m_axis->m_dispFirstEvent;
    int lastEvent = m_axis->m_dispLastEvent;
    const std::vector<GLfloat>& v = static_cast<BarGraphData*>(m_data)->dataY()->data;

    double stride = (lastEvent - firstEvent) / (double)(m_numElements-1);
    if (firstEvent == lastEvent) {
        return (!isEventFiltered(lastEvent)) ? v[lastEvent] : 0;
    } else if (firstEvent > lastEvent) {
        return 0;
    }

    GLfloat largest = (!isEventFiltered(firstEvent)) ? v[firstEvent] : 0;
    for (uint i=1; i < m_numElements; ++i) {
        int index = round(firstEvent + i * stride);
        if (isEventFiltered(index)) continue;
        float tested = v[index];
        if (largest < tested) {
            largest = tested;
        }
    }
    return largest;
}

void BarGraph::updateMaxY() {
    if (!m_numElements || !m_axis || !m_data) return;
    if (m_ignoreUpdates) {
        m_needsUpdatingMaxY = true;
        return;
    }
    MaxYUpdater* task = new MaxYUpdater(*this, m_axis->m_dispFirstEvent,
                        m_axis->m_dispLastEvent, m_numElements);
    connect(task, &MaxYUpdater::maxYObtained, this, &BarGraph::setMaxY);
    QThreadPool::globalInstance()->start(task);
}


QOpenGLShaderProgram* BarGraphRenderer::m_programs[BarGraphRenderer::PROGRAM_SIZE] = { nullptr };
QGLBuffer BarGraphRenderer::m_vertexBuffer = QGLBuffer(QGLBuffer::VertexBuffer);
GLuint BarGraphRenderer::m_vao = 0;
unsigned BarGraphRenderer::m_numInstances = 0;

BarGraphRenderer::BarGraphRenderer(BarGraph* item)
    : AbstractGraphRenderer(item), m_doublePrecisionCopy(item->m_doublePrecision)
{
    m_numInstances++;
}

BarGraphRenderer::~BarGraphRenderer() {
    if (!(--m_numInstances)) {
        glDeleteVertexArrays(1, &m_vao);
        for (auto& p : m_programs) {
            if (p) delete p;
            p = nullptr;
        }
    }
}

void BarGraphRenderer::synchronizeAfter(QQuickFramebufferObject* item) {
    BarGraph* i = static_cast<BarGraph*>(item);

    if (i->m_needsUpdatingMaxY) {
        i->updateMaxY();
        i->m_needsUpdatingMaxY = false;
    }

    m_maxYCopy = i->m_maxY;
    m_doublePrecisionCopy = i->m_doublePrecision;
}

void BarGraphRenderer::render() {
    typedef void (*glUniform1dType)(int, double);
    static glUniform1dType glUniform1d = nullptr;
    static QOpenGLShader *fp32vshader = nullptr, *fp64vshader = nullptr,
                          *filtfshader = nullptr, *nofiltfshader = nullptr;
    if (!m_GLinit) {
        initializeOpenGLFunctions();
        if (m_win->openglContext()->hasExtension("GL_ARB_gpu_shader_fp64")) {
            m_doublePrecisionAvailable = true;
            glUniform1d = reinterpret_cast<glUniform1dType>(
                m_win->openglContext()->getProcAddress("glUniform1d")
            );
        }
        m_GLinit = true;
    }
    if (!m_programs[PROGRAM_NOFILT_FP32]) {
        fp32vshader = new QOpenGLShader(QOpenGLShader::Vertex,
                        m_programs[PROGRAM_NOFILT_FP32]);
        fp32vshader->compileSourceCode(
                "#version 140\n"
                "in vec2 vertex;\n"
                "uniform usamplerBuffer texXH;\n"
                "uniform usamplerBuffer texXL;\n"
                "uniform samplerBuffer texW;\n"
                "uniform samplerBuffer texY;\n"
                "uniform float scaleInv;\n"
                "uniform float relStartTime;\n"
                "uniform vec2 eventWindow;\n"
                "uniform int numEvents;\n"
                "uniform float maxY;\n"
                "uniform int windowSize;\n"
                "flat out int index;\n"
                "void main(void)\n"
                "{\n"
                "   index = int(round(eventWindow[0] + float(gl_InstanceID) / (numEvents-1)  * (eventWindow[1]-eventWindow[0])));\n"
                "   float xScale = texelFetch(texW, index).r / scaleInv;\n"
                "   xScale = max(xScale, 1. / windowSize );\n" // 1 pixel min
                "   float yScale = texelFetch(texY, index).r / maxY;\n"
                "   vec2 resized = mat2(xScale, 0, 0, yScale) * vertex;\n"
                "   vec2 displaced = resized + vec2(xScale,yScale);\n"
                "   float positionH = (texelFetch(texXH,index).r /scaleInv) * 1e-9;\n"
                "   positionH *= pow(2.,32.);\n"
                "   float positionL = (texelFetch(texXL,index).r/scaleInv) * 1e-9;\n"
                "   vec2 positioned = displaced + vec2(2.*positionL, 0);\n" // smaller values go first
                "   positioned += vec2(2.*(positionH-relStartTime) - 1, -1);\n"
                "   gl_Position = vec4(positioned,0,1);\n"
                "}");
        if (m_doublePrecisionAvailable) {
        fp64vshader = new QOpenGLShader(QOpenGLShader::Vertex,
                        m_programs[PROGRAM_NOFILT_FP64]);
        fp64vshader->compileSourceCode(
                "#version 140\n"
                "#extension GL_ARB_gpu_shader_fp64 : enable\n"
                "in vec2 vertex;\n"
                "uniform usamplerBuffer texXH;\n"
                "uniform usamplerBuffer texXL;\n"
                "uniform samplerBuffer texW;\n"
                "uniform samplerBuffer texY;\n"
                "uniform double scaleInv;\n"
                "uniform double relStartTime;\n"
                "uniform vec2 eventWindow;\n"
                "uniform int numEvents;\n"
                "uniform float maxY;\n"
                "uniform int windowSize;\n"
                "flat out int index;\n"
                "void main(void)\n"
                "{\n"
                "   index = int(round(eventWindow[0] + float(gl_InstanceID) / (numEvents-1)  * (eventWindow[1]-eventWindow[0])));\n"
                "   double xScale = texelFetch(texW, index).r * scaleInv;\n"
                "   xScale = max(xScale, 1. / windowSize );\n" // 1 pixel min
                "   float yScale = texelFetch(texY, index).r / maxY;\n"
                "   dvec2 resized = dmat2(xScale, 0, 0, yScale) * vertex;\n"
                "   dvec2 displaced = resized + dvec2(xScale,yScale);\n"
                "   double positionH = (texelFetch(texXH,index).r *scaleInv) * 4294967296e-9lf;\n"
                //"   positionH *= 4.294967296e0lf;\n"
                "   double positionL = (texelFetch(texXL,index).r*scaleInv) * 1e-9lf;\n"
                "   dvec2 positioned = displaced + dvec2(2.*(positionH+positionL-relStartTime) - 1, -1);\n"
                "   gl_Position = vec4(positioned,0,1);\n"
                "}");
        }
        filtfshader = new QOpenGLShader(QOpenGLShader::Fragment,
                        m_programs[PROGRAM_NOFILT_FP32]);
        filtfshader->compileSourceCode(
                "#version 140\n"
                "uniform vec4 color;\n"
                "uniform usamplerBuffer texFilter;\n"
                "uniform int filterId;\n"
                "flat in int index;\n"
                "out vec4 outColor;\n"
                "void main(void)\n"
                "{\n"
                "	if (texelFetch(texFilter,index).r != uint(filterId))\n"
                "		discard;\n"
                "   outColor = color;\n"
                "}");
        nofiltfshader = new QOpenGLShader(QOpenGLShader::Fragment,
                            m_programs[PROGRAM_NOFILT_FP32]);
        nofiltfshader->compileSourceCode(
                "#version 140\n"
                "uniform vec4 color;\n"
                "flat in int index;\n"
                "out vec4 outColor;\n"
                "void main(void)\n"
                "{\n"
                "   outColor = color;\n"
                "}");

        for (int i = (m_doublePrecisionAvailable?0:PROGRAM_SIZE/2); i < PROGRAM_SIZE; i++)
        {
            m_programs[i] = new QOpenGLShaderProgram();
        }
        if (m_doublePrecisionAvailable) {
            m_programs[PROGRAM_NOFILT_FP64]->addShader(fp64vshader);
            m_programs[PROGRAM_FILT_FP64]->addShader(fp64vshader);

            m_programs[PROGRAM_NOFILT_FP64]->addShader(nofiltfshader);
            m_programs[PROGRAM_FILT_FP64]->addShader(filtfshader);
        }

        m_programs[PROGRAM_NOFILT_FP32]->addShader(fp32vshader);
        m_programs[PROGRAM_FILT_FP32]->addShader(fp32vshader);

        m_programs[PROGRAM_NOFILT_FP32]->addShader(nofiltfshader);
        m_programs[PROGRAM_FILT_FP32]->addShader(filtfshader);

        for (int i = (m_doublePrecisionAvailable?0:PROGRAM_SIZE/2); i < PROGRAM_SIZE; i++)
        {
            m_programs[i]->link();
        }

        static GLfloat const quad[] = {
            -1.0f,  -1.0f,
            1.0f,  -1.0f,
            -1.0f,  1.0f,
            1.0f,  1.0f
        };
        m_vertexBuffer.create();
        m_vertexBuffer.setUsagePattern(QGLBuffer::StaticDraw);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        m_vertexBuffer.bind();
        m_vertexBuffer.allocate(quad, 4 * 2 * sizeof(float));
        for (int i = (m_doublePrecisionAvailable?0:PROGRAM_SIZE/2); i < PROGRAM_SIZE; i++)
        {
            m_programs[i]->setAttributeBuffer("vertex", GL_FLOAT, 0, 2);
            m_programs[i]->enableAttributeArray("vertex");
        }
        m_vertexBuffer.release();

        glBindVertexArray(0);
    }

    int programNum;
    if (m_filteredCopy) {
        programNum = PROGRAM_FILT_FP64;
    } else {
        programNum = PROGRAM_NOFILT_FP64;
    }
    if (!m_doublePrecisionCopy) programNum += PROGRAM_SIZE / 2;
    m_program = m_programs[programNum];

    if (!m_needsUpdating || !m_axisCopy || !m_dataCopy) return;

    m_program->bind();
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    m_axisCopy->xH->bindTexture();
    glActiveTexture(GL_TEXTURE1);
    m_axisCopy->xL->bindTexture();
    glActiveTexture(GL_TEXTURE2);
    m_axisCopy->xW->bindTexture();
    glActiveTexture(GL_TEXTURE3);
    static_cast<BarGraphData*>(m_dataCopy)->dataY()->bindTexture();
    if (m_filteredCopy) {
        glActiveTexture(GL_TEXTURE4);
        m_dataCopy->dataFilter()->bindTexture();
    }

    int colorLocation = m_program->uniformLocation("color");
    QColor color(255, 124, 124, 255);

    uint firstDispItemId = m_dispFirstEventCopy;
    uint lastDispItemId  = m_dispLastEventCopy;
    int numEvents = std::min(m_numElementsCopy, lastDispItemId-firstDispItemId+1);
    numEvents = std::max(numEvents, 2);

    m_program->setUniformValue(colorLocation, color);
    m_program->setUniformValue("texXH", 0);
    m_program->setUniformValue("texXL", 1);
    m_program->setUniformValue("texW", 2);
    m_program->setUniformValue("texY", 3);
    if (m_filteredCopy) {
        m_program->setUniformValue("texFilter", 4);
        m_program->setUniformValue("filterId", (GLint)m_filterCopy);
    }
    if (!m_doublePrecisionCopy) {
    m_program->setUniformValue("scaleInv", (m_dispEndTimeCopy - m_dispStartTimeCopy) * (float)1e-9);
    m_program->setUniformValue("relStartTime",
        (float) (m_dispStartTimeCopy / (double)(m_dispEndTimeCopy - m_dispStartTimeCopy)) );
    } else {
    glUniform1d(m_program->uniformLocation("scaleInv"),
               1.e9 / (m_dispEndTimeCopy - m_dispStartTimeCopy) );
    glUniform1d(m_program->uniformLocation("relStartTime"),
        (double)m_dispStartTimeCopy / (double)(m_dispEndTimeCopy - m_dispStartTimeCopy) );
    }
    m_program->setUniformValue("eventWindow", firstDispItemId, lastDispItemId);
    m_program->setUniformValue("numEvents", numEvents);
    m_program->setUniformValue("maxY", m_maxYCopy );
    m_program->setUniformValue("windowSize", (GLint)m_width);

    glDisable(GL_DEPTH_TEST);

    glClearColor(m_bgcolorCopy.redF(), m_bgcolorCopy.greenF(),
                 m_bgcolorCopy.blueF(), m_bgcolorCopy.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numEvents);

    glBindVertexArray(0);

    m_program->release();
    m_needsUpdating = false;
    m_win->resetOpenGLState();
}
