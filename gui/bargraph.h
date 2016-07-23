#include <QOpenGLFunctions_3_1>
#include <QOpenGLShaderProgram>
#include <QGLBuffer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickFramebufferObject>

#include <iostream>
#include <memory>
#include <set>

#pragma once

template<typename T>
class TextureBufferData : protected QOpenGLFunctions_3_1
{
public:
    TextureBufferData(const std::vector<T>& v) : data(v), buffer(0), texture(0) {}

    void init() {
        if (!buffer) {
            initializeOpenGLFunctions();
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_TEXTURE_BUFFER, buffer);
            glBufferData(GL_TEXTURE_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
            glGenTextures(1, &texture);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_BUFFER, texture);
            texBuffer();
        }
    }

    void deinit() {
        if (buffer) {
            glDeleteBuffers(1, &buffer);
            glDeleteTextures(1, &texture);
        }
    }

    void bindTexture() {
        glBindTexture(GL_TEXTURE_BUFFER, texture);
    }

    const std::vector<T>& data;

private:
    void texBuffer();

    GLuint buffer;
    GLuint texture;
};
template<typename T>
inline void TextureBufferData<T>::texBuffer() { glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer); }
template<>
inline void TextureBufferData<GLuint>::texBuffer() { glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, buffer); }


class TimelineAxis : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qulonglong startTime READ startTime NOTIFY startTimeChanged)
    Q_PROPERTY(qulonglong endTime READ endTime NOTIFY endTimeChanged)
    Q_PROPERTY(qulonglong dispStartTime READ dispStartTime WRITE setDispStartTime NOTIFY dispStartTimeChanged)
    Q_PROPERTY(qulonglong dispEndTime READ dispEndTime WRITE setDispEndTime NOTIFY dispEndTimeChanged)

public:
    TimelineAxis(const std::shared_ptr<TextureBufferData<GLuint>>& xH,
                 const std::shared_ptr<TextureBufferData<GLuint>>& xL,
                 const std::shared_ptr<TextureBufferData<GLfloat>>& xW)
        : xH(xH), xL(xL), xW(xW), m_firstEvent(0), m_lastEvent(xH->data.size()-1)
    {
        m_startTime = (((uint64_t)xH->data[0] << 32) + xL->data[0] );
        m_endTime = (((uint64_t)xH->data[m_lastEvent] << 32) + xL->data[m_lastEvent]);
    }

    inline void init() {if (!m_init) { xH->init(); xL->init(); xW->init(); m_init=true;} }
    inline void deinit() { if (m_init) {xH->deinit(); xL->deinit(); xW->deinit(); m_init=false;} }

    Q_INVOKABLE int findEventAtTime(qlonglong time) const;
    Q_INVOKABLE qlonglong eventStartTime(int id) const;
    Q_INVOKABLE qlonglong eventDurationTime(int id) const;

    uint64_t startTime() const { return m_startTime; }
    uint64_t endTime() const { return m_endTime; }
    uint64_t dispStartTime() const { return m_dispStartTime; }
    void setDispStartTime(uint64_t time) { m_dispStartTime = time; mapEvents(); emit dispStartTimeChanged(); }
    uint64_t dispEndTime() const { return m_dispEndTime; }
    void setDispEndTime(uint64_t time) { m_dispEndTime = time; mapEvents(); emit dispEndTimeChanged(); }

    std::shared_ptr<TextureBufferData<GLuint> > xH;
    std::shared_ptr<TextureBufferData<GLuint> > xL;
    std::shared_ptr<TextureBufferData<GLfloat> > xW;
    GLuint m_dispFirstEvent;
    GLuint m_dispLastEvent;

signals:
    void startTimeChanged();
    void endTimeChanged();
    void dispStartTimeChanged();
    void dispEndTimeChanged();

private:
    void mapEvents();

    bool m_init = false;
    uint64_t m_startTime;
    uint64_t m_endTime;
    uint64_t m_dispStartTime;
    uint64_t m_dispEndTime;
    GLuint m_firstEvent;
    GLuint m_lastEvent;
};


class BarGraphData : public QObject
{
    Q_OBJECT

public:
    BarGraphData(const std::shared_ptr<TextureBufferData<GLfloat>>& dataY,
                 const std::shared_ptr<TextureBufferData<GLuint>>& dataFilter)
        : m_dataY(dataY), m_dataFilter(dataFilter) {}

    TextureBufferData<GLfloat>* dataY() const { return m_dataY.get();}
    TextureBufferData<GLuint>* dataFilter() const { return m_dataFilter.get();}
    Q_INVOKABLE float eventYValue(int id) const;

private:
    std::shared_ptr<TextureBufferData<GLfloat>> m_dataY;
    std::shared_ptr<TextureBufferData<GLuint>> m_dataFilter;
};

class BarGraph;
class BarGraphRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_1
{
public:
    BarGraphRenderer(BarGraph* item);
    ~BarGraphRenderer();

    void setViewport(const QSize &size) { m_viewport = size; }

public:
    void render();
    void synchronize(QQuickFramebufferObject* item);

private:
    bool m_GLinit = false;
    bool m_doublePrecision = false;
    static QOpenGLShaderProgram* m_program;
    BarGraph* m_item;
    static QGLBuffer m_vertexBuffer;
    static GLuint m_vao;
    QSize m_viewport;
};


class BarGraph : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(uint numElements READ numElements WRITE setNumElements NOTIFY numElementsChanged)
    Q_PROPERTY(uint rendTime READ rendTime WRITE setRendTime NOTIFY rendTimeChanged)
    Q_PROPERTY(TimelineAxis* axis READ axis WRITE setAxis NOTIFY axisChanged)
    Q_PROPERTY(BarGraphData* data READ data WRITE setData)
    Q_PROPERTY(uint filter MEMBER m_filter)
    Q_PROPERTY(QColor bgcolor MEMBER m_bgcolor)
    Q_PROPERTY(float maxY READ maxY WRITE setMaxY NOTIFY maxYChanged)

public:
    BarGraph(QQuickItem *parent = 0);

    uint numElements() const { return m_numElements; }
    void setNumElements(uint num) { if (m_numElements != num) {m_numElements = num; forceupdate();} }

    uint rendTime() const { return m_rendTime; }
    void setRendTime(uint time) { m_rendTime = time; emit rendTimeChanged(); }

    TimelineAxis* axis() const { return m_axis; }
    void setAxis(TimelineAxis* axis) { m_axis = axis; }

    BarGraphData* data() const { return m_data; }
    void setData(BarGraphData* data) { m_data = data; }

    float maxY() const { return m_maxY; }
    void setMaxY(float maxy) { m_maxY = maxy; emit maxYChanged(); }

    QQuickFramebufferObject::Renderer* createRenderer() const;

    Q_INVOKABLE bool isEventFiltered(int id) const;
    Q_INVOKABLE uint eventFilter(int id) const;

    float maxVisibleEvent() const;

    class range_iterator {
    public:
        range_iterator(BarGraph* bg, uint64_t begin, uint64_t duration);
        int operator++();
        int index() const { return curElement; }

    private:
        BarGraph* ptr;
        int curElement;
        uint64_t end;
    };

signals:
    void fileChanged();
    void numElementsChanged();
    void rendTimeChanged();
    void axisChanged();
    void maxYChanged();

public slots:
    void forceupdate();
    void initRenderer();
    void cleanupRenderer();

private:
    BarGraphRenderer* m_renderer;

public:
    bool m_needsUpdating;
    TimelineAxis* m_axis;
    BarGraphData*  m_data;

    uint m_numElements;
    GLfloat m_maxY;
    GLuint m_rendTime;

    GLuint m_filter;
    QColor m_bgcolor;
};

class SetIterHelper : public QObject
{
    Q_OBJECT

public:
    SetIterHelper(std::set<GLuint>& set) : m_set(set) {}
    Q_PROPERTY(uint size READ size)
    Q_PROPERTY(int value READ value)

    void reset() {
        it = m_set.begin();
    }

    GLint value() {
        return *it;
    }

    Q_INVOKABLE void next() {
        if (++it == m_set.end()) {
            reset();
        }
    }

    GLuint size() {
        return m_set.size();
    }

private:
    std::set<GLuint>& m_set;
    std::set<GLuint>::iterator it;
};

