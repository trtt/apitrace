#include "bargraph.h"

#include <QQuickWindow>
#include <QElapsedTimer>

#include <iostream>
#include <cmath>
#include <chrono>


BarGraph::BarGraph(QQuickItem *parent)
    : QQuickFramebufferObject(parent),  m_renderer(0), m_needsUpdating(true),
      m_numElements(0), m_filter(0), m_bgcolor("white")
{
    setMirrorVertically(true);
}

QQuickFramebufferObject::Renderer* BarGraph::createRenderer() const {
    return new BarGraphRenderer(const_cast<BarGraph*>(this));
}

void BarGraph::forceupdate() {
    m_needsUpdating = true;
    update();
}

void BarGraph::initRenderer() {
    connect(m_axis, &TimelineAxis::dispStartTimeChanged, this, &BarGraph::forceupdate);
    connect(m_axis, &TimelineAxis::dispEndTimeChanged, this, &BarGraph::forceupdate);
    m_axis->init();
    m_data->dataY()->init();
    m_data->dataFilter()->init();
}

void BarGraph::cleanupRenderer() {
    m_axis->deinit();
    m_data->dataY()->deinit();
    m_data->dataFilter()->deinit();
}

QOpenGLShaderProgram* BarGraphRenderer::m_program = nullptr;
QGLBuffer BarGraphRenderer::m_vertexBuffer = QGLBuffer(QGLBuffer::VertexBuffer);
GLuint BarGraphRenderer::m_vao = 0;

BarGraphRenderer::BarGraphRenderer(BarGraph* item) : m_item(item) {
    m_item->initRenderer();
}

BarGraphRenderer::~BarGraphRenderer() {
    m_item->cleanupRenderer();
    glDeleteVertexArrays(1, &m_vao);
    if (m_program) {
        delete m_program;
        m_program = nullptr;
    }
}

static int upper_bound64(int begin, int end, const std::vector<GLuint>& dataH,
                         const std::vector<GLuint>& dataL,
                         int stride, uint64_t val)
{
    int count = end - begin;
    while (count > 0) {
        int step = (count / stride ) / 2 * stride;
        uint64_t curValue = ((uint64_t)dataH[begin+step] << 32) + dataL[begin+step];
        if (!(val < curValue)) {
            begin += step + stride;
            count -= step + stride;
        } else {
            count = step;
        }
    }
    return begin;
}

void TimelineAxis::mapEvents() {
    m_dispFirstEvent = upper_bound64(m_firstEvent, m_lastEvent,
                                         xH->data, xL->data,
                                         1, m_dispStartTime) - 1;
    m_dispLastEvent  = upper_bound64(m_firstEvent, m_lastEvent,
                                         xH->data, xL->data,
                                         1, m_dispEndTime) - 1;
}

int TimelineAxis::findEventAtTime(qlonglong time) const {
    return upper_bound64(m_dispFirstEvent, m_dispLastEvent+1, xH->data, xL->data,
                         1, time) - 1;
}

qlonglong TimelineAxis::eventStartTime(int id) const {
     return ((uint64_t)xH->data[id] << 32) + xL->data[id];
}

qlonglong TimelineAxis::eventDurationTime(int id) const {
     return (double)xW->data[id] * 1e9;
}


float BarGraphData::eventYValue(int id) const {
     return m_dataY->data[id];
}


bool BarGraph::isEventFiltered(int id) const {
    return (m_data->dataFilter()->data[id] != m_filter);
}

uint BarGraph::eventFilter(int id) const {
    return m_filter;
}

BarGraph::range_iterator::range_iterator(BarGraph* bg, uint64_t begin,
                                         uint64_t duration)
    : ptr(bg), end(begin+duration)
{
    curElement = upper_bound64(ptr->m_axis->m_dispFirstEvent,
                               ptr->m_axis->m_dispLastEvent,
                               ptr->m_axis->xH->data, ptr->m_axis->xL->data,
                               1, begin) - 1;
    operator++();
    if ( ((uint64_t)ptr->m_axis->xH->data[curElement] << 32) \
         + ptr->m_axis->xL->data[curElement] < begin) curElement = -1;
}

int BarGraph::range_iterator::operator++() {
    if (curElement == -1) return -1;
    do {
        ++curElement;
        uint64_t elementEnd = ((uint64_t)ptr->m_axis->xH->data[curElement] << 32) \
                              + ptr->m_axis->xL->data[curElement] \
                              + ptr->m_axis->xW->data[curElement] * (double)1e9;
        if (elementEnd > end) {
            curElement = -1;
            break;
        }
    } while (ptr->m_data->dataFilter()->data[curElement] != ptr->m_filter);
    return curElement;
}

float BarGraph::maxVisibleEvent() const {
    int firstEvent = m_axis->m_dispFirstEvent;
    int lastEvent = m_axis->m_dispLastEvent;
    const std::vector<GLfloat>& v = m_data->dataY()->data;

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

GLfloat max_element_strided(const std::vector<GLfloat>& v,
                           uint firstEvent, uint lastEvent, uint numEvents)
{
    float stride = (lastEvent - firstEvent) / (numEvents-1);
    if (firstEvent == lastEvent) {
        return v[lastEvent];
    } else if (firstEvent > lastEvent) {
        return 0;
    }
    GLfloat largest = v[firstEvent];
    for (uint i=1; i < numEvents; ++i) {
        float tested = v[round(firstEvent + i * stride)];
        if (largest < tested) {
            largest = tested;
        }
    }
    return largest;
}

void BarGraphRenderer::synchronize(QQuickFramebufferObject* item) {

}

void BarGraphRenderer::render() {
    typedef void (*glUniform1dType)(int, double);
    static glUniform1dType glUniform1d = nullptr;
    if (!m_GLinit) {
        initializeOpenGLFunctions();
        if (m_item->window()->openglContext()->hasExtension("GL_ARB_gpu_shader_fp64")) {
            m_doublePrecision = true;
            glUniform1d = reinterpret_cast<glUniform1dType>(
                m_item->window()->openglContext()->getProcAddress("glUniform1d")
            );
        }
        m_GLinit = true;
    }
    if (!m_program) {
        qDebug() << QString(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

        m_program = new QOpenGLShaderProgram();
        if (!m_doublePrecision) {
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
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
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                "#version 430\n"
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
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
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

        m_program->link();

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
        m_program->setAttributeBuffer("vertex", GL_FLOAT, 0, 2);
        m_program->enableAttributeArray("vertex");
        m_vertexBuffer.release();

        glBindVertexArray(0);
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    if (!m_item->m_needsUpdating) return;
    QPointF sceneCoord = m_item->parentItem()->mapToScene(QPointF(m_item->x(), m_item->y()));
    if ((sceneCoord.y() + m_item->height() < 0) ||
        (sceneCoord.y() > m_item->window()->size().height())) return;

    m_program->bind();
    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    m_item->m_axis->xH->bindTexture();
    glActiveTexture(GL_TEXTURE1);
    m_item->m_axis->xL->bindTexture();
    glActiveTexture(GL_TEXTURE2);
    m_item->m_axis->xW->bindTexture();
    glActiveTexture(GL_TEXTURE3);
    m_item->data()->dataY()->bindTexture();
    glActiveTexture(GL_TEXTURE4);
    m_item->data()->dataFilter()->bindTexture();

    int colorLocation = m_program->uniformLocation("color");
    QColor color(255, 124, 124, 255);

    uint firstDispItemId = m_item->m_axis->m_dispFirstEvent;
    uint lastDispItemId  = m_item->m_axis->m_dispLastEvent;
    int numEvents = std::min(m_item->numElements(), lastDispItemId-firstDispItemId+1);
    numEvents = std::max(numEvents, 2);
    //float maxY = max_element_strided(m_item->data()->dataY()->data, firstDispItemId,
                                     //lastDispItemId, numEvents);
    float maxY = m_item->maxVisibleEvent();
    if (std::abs(maxY) < std::numeric_limits<float>::epsilon())
        maxY = 1.;
    m_item->setMaxY(maxY);

    m_program->setUniformValue(colorLocation, color);
    m_program->setUniformValue("texXH", 0);
    m_program->setUniformValue("texXL", 1);
    m_program->setUniformValue("texW", 2);
    m_program->setUniformValue("texY", 3);
    m_program->setUniformValue("texFilter", 4);
    m_program->setUniformValue("filterId", (GLint)m_item->m_filter);
    if (!m_doublePrecision) {
    m_program->setUniformValue("scaleInv", (m_item->m_axis->dispEndTime() - m_item->m_axis->dispStartTime()) * (float)1e-9);
    m_program->setUniformValue("relStartTime",
        (float) (m_item->m_axis->dispStartTime() / (double)(m_item->m_axis->dispEndTime() - m_item->m_axis->dispStartTime())) );
    } else {
    glUniform1d(m_program->uniformLocation("scaleInv"),
               1.e9 / (m_item->m_axis->dispEndTime() - m_item->m_axis->dispStartTime()) );
    glUniform1d(m_program->uniformLocation("relStartTime"),
        (double)m_item->m_axis->dispStartTime() / (double)(m_item->m_axis->dispEndTime() - m_item->m_axis->dispStartTime()) );
    }
    m_program->setUniformValue("eventWindow", firstDispItemId, lastDispItemId);
    m_program->setUniformValue("numEvents", numEvents);
    m_program->setUniformValue("maxY", maxY );
    m_program->setUniformValue("windowSize", (GLint)m_item->width());

    //std::cout << "Draw operation!" << std::endl;
    //std::cout << m_item->m_dispStartTime << " - " << firstDispItemId << std::endl;
    //std::cout << m_item->m_dispEndTime << " - " << lastDispItemId  << std::endl;
    //std::cout << numEvents << std::endl;
    //std::cout << maxY << std::endl;

    glDisable(GL_DEPTH_TEST);

    glClearColor(m_item->m_bgcolor.redF(), m_item->m_bgcolor.greenF(),
                 m_item->m_bgcolor.blueF(), m_item->m_bgcolor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numEvents);

    glBindVertexArray(0);


    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> fp_ms = t2 - t1;
    m_item->setRendTime(fp_ms.count());


    m_program->release();
    m_item->m_needsUpdating = false;
    m_item->window()->resetOpenGLState();
}
