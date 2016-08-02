#include "bargraph.h"

#include <QQuickWindow>

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


BarGraph::BarGraph(QQuickItem *parent)
    : m_maxY(1.f)
{
}

QQuickFramebufferObject::Renderer* BarGraph::createRenderer() const {
    return new BarGraphRenderer(const_cast<BarGraph*>(this));
}

void BarGraph::forceupdate() {
    updateMaxY();
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
    if (m_offscreen) {
        m_needsUpdatingMaxY = true;
        return;
    }
    float maxY = maxVisibleEvent();
    if (std::abs(maxY) < std::numeric_limits<float>::epsilon())
        maxY = 1.;
    setMaxY(maxY);
}


QOpenGLShaderProgram* BarGraphRenderer::m_filtProgram = nullptr;
QOpenGLShaderProgram* BarGraphRenderer::m_nofiltProgram = nullptr;
QGLBuffer BarGraphRenderer::m_vertexBuffer = QGLBuffer(QGLBuffer::VertexBuffer);
GLuint BarGraphRenderer::m_vao = 0;
unsigned BarGraphRenderer::m_numInstances = 0;

BarGraphRenderer::BarGraphRenderer(BarGraph* item) : AbstractGraphRenderer(item) {
    m_numInstances++;
}

BarGraphRenderer::~BarGraphRenderer() {
    if (!(--m_numInstances)) {
        glDeleteVertexArrays(1, &m_vao);
        delete m_filtProgram;
        delete m_nofiltProgram;
        m_filtProgram = nullptr;
        m_nofiltProgram = nullptr;
    }
}

void BarGraphRenderer::synchronizeAfter(QQuickFramebufferObject* item) {
    BarGraph* i = static_cast<BarGraph*>(item);

    if (i->m_needsUpdatingMaxY) {
        i->updateMaxY();
        i->m_needsUpdatingMaxY = false;
    }

    m_maxYCopy = i->m_maxY;
}

void BarGraphRenderer::render() {
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
        qDebug() << QString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        if (!m_doublePrecision) {
        fp32vshader = new QOpenGLShader(QOpenGLShader::Vertex, m_filtProgram);
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
        } else {
        fp64vshader = new QOpenGLShader(QOpenGLShader::Vertex, m_filtProgram);
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
        filtfshader = new QOpenGLShader(QOpenGLShader::Fragment, m_filtProgram);
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
        nofiltfshader = new QOpenGLShader(QOpenGLShader::Fragment, m_filtProgram);
        nofiltfshader->compileSourceCode(
                "#version 140\n"
                "uniform vec4 color;\n"
                "flat in int index;\n"
                "out vec4 outColor;\n"
                "void main(void)\n"
                "{\n"
                "   outColor = color;\n"
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
