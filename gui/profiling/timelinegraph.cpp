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

#include "timelinegraph.h"

#include <QQuickWindow>

#include <cmath>


void TimelineGraphData::acquireResHandle() {
    if (!m_numHandles) {
        m_dataColor->init();
    }
    AbstractGraphData::acquireResHandle();
}

void TimelineGraphData::freeResHandle() {
    AbstractGraphData::freeResHandle();
    if (!m_numHandles) {
        if (m_dataColor.unique())
            m_dataColor->deinit();
    }
}


QQuickFramebufferObject::Renderer* TimelineGraph::createRenderer() const {
    return new TimelineGraphRenderer(const_cast<TimelineGraph*>(this));
}


QOpenGLShaderProgram* TimelineGraphRenderer::m_filtProgram = nullptr;
QOpenGLShaderProgram* TimelineGraphRenderer::m_nofiltProgram = nullptr;
QGLBuffer TimelineGraphRenderer::m_vertexBuffer = QGLBuffer(QGLBuffer::VertexBuffer);
GLuint TimelineGraphRenderer::m_vao = 0;
unsigned TimelineGraphRenderer::m_numInstances = 0;

TimelineGraphRenderer::TimelineGraphRenderer(TimelineGraph* item) : AbstractGraphRenderer(item) {
    m_numInstances++;
}

TimelineGraphRenderer::~TimelineGraphRenderer() {
    if (!(--m_numInstances)) {
        glDeleteVertexArrays(1, &m_vao);
        delete m_filtProgram;
        delete m_nofiltProgram;
        m_filtProgram = nullptr;
        m_nofiltProgram = nullptr;
    }
}

void TimelineGraphRenderer::render() {
    typedef void (*glUniform1dType)(int, double);
    static glUniform1dType glUniform1d = nullptr;
    static QOpenGLShader *fp32vshader = nullptr, *fp64vshader = nullptr,
                          *filtfshader = nullptr, *nofiltfshader = nullptr;
    if (!m_GLinit) {
        initializeOpenGLFunctions();
        if (m_win->openglContext()->hasExtension("GL_ARB_gpu_shader_fp64")) {
            m_doublePrecision = true;
            glUniform1d = reinterpret_cast<glUniform1dType>(
                m_win->openglContext()->getProcAddress("glUniform1d")
            );
        }
        m_GLinit = true;
    }
    if (!m_filtProgram) {
        if (!m_doublePrecision) {
        fp32vshader = new QOpenGLShader(QOpenGLShader::Vertex, m_filtProgram);
        fp32vshader->compileSourceCode(
                "#version 140\n"
                "in vec2 vertex;\n"
                "uniform usamplerBuffer texXH;\n"
                "uniform usamplerBuffer texXL;\n"
                "uniform samplerBuffer texW;\n"
                "uniform float scaleInv;\n"
                "uniform float relStartTime;\n"
                "uniform vec2 eventWindow;\n"
                "uniform int numEvents;\n"
                "uniform int windowSize;\n"
                "flat out int index;\n"
                "void main(void)\n"
                "{\n"
                "   index = int(round(eventWindow[0] + float(gl_InstanceID) / (numEvents-1)  * (eventWindow[1]-eventWindow[0])));\n"
                "   float xScale = texelFetch(texW, index).r / scaleInv;\n"
                "   xScale = max(xScale, 1. / windowSize );\n" // 1 pixel min
                "   vec2 resized = mat2(xScale, 0, 0, 1.0) * vertex;\n"
                "   vec2 displaced = resized + vec2(xScale,1.0);\n"
                "   float positionH = (texelFetch(texXH,index).r /scaleInv) * 1e-9;\n"
                "   positionH *= pow(2.,32.);\n"
                "   float positionL = (texelFetch(texXL,index).r/scaleInv) * 1e-9;\n"
                "   vec2 positioned = displaced + vec2(2.*positionL, 0);\n" // smaller values go first
                "   positioned += vec2(2.*(positionH-relStartTime) - 1, -1);\n"
                "   gl_Position = vec4(positioned,0,1);\n"
                "}");
        } else {
        fp64vshader = new QOpenGLShader(QOpenGLShader::Vertex, m_filtProgram);
        fp64vshader->compileSourceCode(
                "#version 140\n"
                "#extension GL_ARB_gpu_shader_fp64 : enable\n"
                "in vec2 vertex;\n"
                "uniform usamplerBuffer texXH;\n"
                "uniform usamplerBuffer texXL;\n"
                "uniform samplerBuffer texW;\n"
                "uniform double scaleInv;\n"
                "uniform double relStartTime;\n"
                "uniform vec2 eventWindow;\n"
                "uniform int numEvents;\n"
                "uniform int windowSize;\n"
                "flat out int index;\n"
                "void main(void)\n"
                "{\n"
                "   index = int(round(eventWindow[0] + float(gl_InstanceID) / (numEvents-1)  * (eventWindow[1]-eventWindow[0])));\n"
                "   double xScale = texelFetch(texW, index).r * scaleInv;\n"
                "   xScale = max(xScale, 1. / windowSize );\n" // 1 pixel min
                "   dvec2 resized = dmat2(xScale, 0, 0, 1.0lf) * vertex;\n"
                "   dvec2 displaced = resized + dvec2(xScale,1.0lf);\n"
                "   double positionH = (texelFetch(texXH,index).r *scaleInv) * 4294967296e-9lf;\n"
                //"   positionH *= 4.294967296e0lf;\n"
                "   double positionL = (texelFetch(texXL,index).r*scaleInv) * 1e-9lf;\n"
                "   dvec2 positioned = displaced + dvec2(2.*(positionH+positionL-relStartTime) - 1, -1);\n"
                "   gl_Position = vec4(positioned,0,1);\n"
                "}");
        }
        filtfshader = new QOpenGLShader(QOpenGLShader::Fragment, m_filtProgram);
        filtfshader->compileSourceCode(
                "#version 140\n"
                "uniform vec4 color;\n"
                "uniform float invColorSize;\n"
                "uniform usamplerBuffer texFilter;\n"
                "uniform usamplerBuffer texColor;\n"
                "uniform int filterId;\n"
                "flat in int index;\n"
                "out vec4 outColor;\n"
                "void main(void)\n"
                "{\n"
                "	if (texelFetch(texFilter,index).r != uint(filterId))\n"
                "		discard;\n"
                "   vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);\n"
                "   vec3 p = abs(fract(texelFetch(texColor,index).rrr * invColorSize + K.xyz) * 6.0 - K.www);\n"
                "   outColor = vec4(color.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), color.y), 1.0);\n"
                "}");
        nofiltfshader = new QOpenGLShader(QOpenGLShader::Fragment, m_filtProgram);
        nofiltfshader->compileSourceCode(
                "#version 140\n"
                "uniform vec4 color;\n"
                "uniform float invColorSize;\n"
                "uniform usamplerBuffer texColor;\n"
                "flat in int index;\n"
                "out vec4 outColor;\n"
                "void main(void)\n"
                "{\n"
                "   vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);\n"
                "   vec3 p = abs(fract(texelFetch(texColor,index).rrr * invColorSize + K.xyz) * 6.0 - K.www);\n"
                "   outColor = vec4(color.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), color.y), 1.0);\n"
                "}");

        m_filtProgram = new QOpenGLShaderProgram();
        m_nofiltProgram = new QOpenGLShaderProgram();
        if (m_doublePrecision) {
            m_filtProgram->addShader(fp64vshader);
            m_nofiltProgram->addShader(fp64vshader);
        } else {
            m_filtProgram->addShader(fp32vshader);
            m_nofiltProgram->addShader(fp32vshader);
        }
        m_filtProgram->addShader(filtfshader);
        m_nofiltProgram->addShader(nofiltfshader);

        m_filtProgram->link();
        m_nofiltProgram->link();

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
        m_filtProgram->setAttributeBuffer("vertex", GL_FLOAT, 0, 2);
        m_nofiltProgram->setAttributeBuffer("vertex", GL_FLOAT, 0, 2);
        m_filtProgram->enableAttributeArray("vertex");
        m_nofiltProgram->enableAttributeArray("vertex");
        m_vertexBuffer.release();

        glBindVertexArray(0);
        m_program = m_nofiltProgram;
    }
    if (m_filteredCopy) {
        m_program = m_filtProgram;
    } else {
        m_program = m_nofiltProgram;
    }

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
    static_cast<TimelineGraphData*>(m_dataCopy)->dataColor()->bindTexture();
    if (m_filteredCopy) {
        glActiveTexture(GL_TEXTURE4);
        m_dataCopy->dataFilter()->bindTexture();
    }

    int colorLocation = m_program->uniformLocation("color");
    QColor color(0, 120, 255, 255); // HSV (hue is substituted)

    uint firstDispItemId = m_dispFirstEventCopy;
    uint lastDispItemId  = m_dispLastEventCopy;
    int numEvents = std::min(m_numElementsCopy, lastDispItemId-firstDispItemId+1);
    numEvents = std::max(numEvents, 2);

    m_program->setUniformValue(colorLocation, color);
    m_program->setUniformValue("texXH", 0);
    m_program->setUniformValue("texXL", 1);
    m_program->setUniformValue("texW", 2);
    m_program->setUniformValue("texColor", 3);
    if (m_filteredCopy) {
        m_program->setUniformValue("texFilter", 4);
        m_program->setUniformValue("filterId", (GLint)m_filterCopy);
    }
    if (!m_doublePrecision) {
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
    m_program->setUniformValue("windowSize", (GLint)m_width);
    m_program->setUniformValue("invColorSize",
        (GLfloat) (1. / static_cast<TimelineGraphData*>(m_dataCopy)->colorSize()) );

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
