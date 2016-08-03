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

private:
    bool m_GLinit = false;
    bool m_doublePrecision = false;
    QOpenGLShaderProgram* m_program;
    static QOpenGLShaderProgram* m_filtProgram;
    static QOpenGLShaderProgram* m_nofiltProgram;
    static QGLBuffer m_vertexBuffer;
    static GLuint m_vao;
    static unsigned m_numInstances;
};


class TimelineGraph : public AbstractGraph
{
    Q_OBJECT

    friend class TimelineGraphRenderer;

public:
    TimelineGraph(QQuickItem *parent = 0) {}

    QQuickFramebufferObject::Renderer* createRenderer() const;
};
