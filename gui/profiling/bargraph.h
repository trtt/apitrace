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

    GLfloat m_maxY;
};
