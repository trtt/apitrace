#include "abstractgraph.h"

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
class BarGraphRenderer : public AbstractGraphRenderer
{
public:
    BarGraphRenderer(BarGraph* item);
    ~BarGraphRenderer();

public:
    void render();
    void synchronizeAfter(QQuickFramebufferObject* item);

private:
    bool m_GLinit = false;
    bool m_doublePrecision = false;
    QOpenGLShaderProgram* m_program;
    static QOpenGLShaderProgram* m_filtProgram;
    static QOpenGLShaderProgram* m_nofiltProgram;
    static QGLBuffer m_vertexBuffer;
    static GLuint m_vao;
    static unsigned m_numInstances;

    GLfloat m_maxYCopy;
};


class BarGraph : public AbstractGraph
{
    Q_OBJECT
    Q_PROPERTY(float maxY READ maxY WRITE setMaxY NOTIFY maxYChanged)

    friend class BarGraphRenderer;

public:
    BarGraph(QQuickItem *parent = 0);

    float maxY() const { return m_maxY; }
    void setMaxY(float maxy) { m_maxY = maxy; emit maxYChanged(); }

    QQuickFramebufferObject::Renderer* createRenderer() const;

    float maxVisibleEvent() const;

signals:
    void maxYChanged();

public slots:
    void forceupdate();
    void updateMaxY();

private:
    bool m_needsUpdatingMaxY = false;

    GLfloat m_maxY;
};
