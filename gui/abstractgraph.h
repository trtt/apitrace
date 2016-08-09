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
            buffer = 0;
            texture = 0;
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
        m_dispStartTime = m_startTime;
        m_dispEndTime = m_endTime;
        m_dispFirstEvent = m_firstEvent;
        m_dispLastEvent = m_lastEvent;
    }

    void acquireResHandle();
    void freeResHandle();

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

    unsigned m_numHandles = 0;
    uint64_t m_startTime;
    uint64_t m_endTime;
    uint64_t m_dispStartTime;
    uint64_t m_dispEndTime;
    GLuint m_firstEvent;
    GLuint m_lastEvent;
};


class AbstractGraphData : public QObject
{
    Q_OBJECT

public:
    AbstractGraphData(const std::shared_ptr<TextureBufferData<GLuint>>& dataFilter)
        : m_dataFilter(dataFilter) {}
    virtual ~AbstractGraphData() {}

    TextureBufferData<GLuint>* dataFilter() const { return m_dataFilter.get();}

    virtual void acquireResHandle();
    virtual void freeResHandle();

protected:
    unsigned m_numHandles = 0;

private:
    std::shared_ptr<TextureBufferData<GLuint>> m_dataFilter;
};


class AbstractGraph;
class AbstractGraphRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLFunctions_3_1
{
public:
    AbstractGraphRenderer(AbstractGraph* item);
    virtual ~AbstractGraphRenderer();

public:
    virtual void render() = 0;
    void synchronize(QQuickFramebufferObject* item);
    virtual void synchronizeAfter(QQuickFramebufferObject* item) {}

private:
    AbstractGraph* m_item;

    /*
    * Below are duplicates of AbstractGraph and TimelineAxis (writeable) members
    * using them to avoid race conditions
    * since Renderer is called on rendering thread (not gui)
    */
protected:
    bool m_needsUpdating = false;
    bool m_filteredCopy = false;
    TimelineAxis* m_axisCopy;
    AbstractGraphData*  m_dataCopy;
    uint m_numElementsCopy;
    GLuint m_filterCopy;
    QColor m_bgcolorCopy;

    uint64_t m_dispStartTimeCopy;
    uint64_t m_dispEndTimeCopy;
    GLuint m_dispFirstEventCopy;
    GLuint m_dispLastEventCopy;

    QQuickWindow* m_win;
    qreal m_width;
    qreal m_height;
};


class AbstractGraph : public QQuickFramebufferObject
{
    Q_OBJECT
    Q_PROPERTY(uint numElements READ numElements WRITE setNumElements NOTIFY numElementsChanged)
    Q_PROPERTY(TimelineAxis* axis READ axis WRITE setAxis NOTIFY axisChanged)
    Q_PROPERTY(AbstractGraphData* data READ data WRITE setData)
    Q_PROPERTY(uint filter MEMBER m_filter)
    Q_PROPERTY(QColor bgcolor MEMBER m_bgcolor)
    Q_PROPERTY(bool filtered MEMBER m_filtered)
    Q_PROPERTY(bool needsUpdating MEMBER m_needsUpdating)
    Q_PROPERTY(bool ignoreUpdates MEMBER m_ignoreUpdates)

    friend class AbstractGraphRenderer;

public:
    AbstractGraph(QQuickItem *parent = 0);
    virtual ~AbstractGraph() {}

    uint numElements() const { return m_numElements; }
    void setNumElements(uint num) { if (m_numElements != num) {m_numElements = num; emit numElementsChanged();} }

    TimelineAxis* axis() const { return m_axis; }
    void setAxis(TimelineAxis* axis);

    AbstractGraphData* data() const { return m_data; }
    virtual void setData(AbstractGraphData* data) { m_data = data; }

    virtual QQuickFramebufferObject::Renderer* createRenderer() const = 0;

    Q_INVOKABLE bool isEventFiltered(int id) const;
    Q_INVOKABLE uint eventFilter(int id) const;

    class range_iterator {
    public:
        range_iterator(AbstractGraph* bg, uint64_t begin, uint64_t duration);
        int operator++();
        int index() const { return curElement; }

    private:
        AbstractGraph* ptr;
        int curElement;
        uint64_t end;
    };

signals:
    void numElementsChanged();
    void axisChanged();

public slots:
    virtual void forceupdate();

private:
    AbstractGraphRenderer* m_renderer;
    bool m_filtered;
    bool m_needsUpdating = true;
    GLuint m_filter;
    QColor m_bgcolor;

protected:
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    QSGNode* updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData);
#endif

    bool m_ignoreUpdates = false;
    TimelineAxis* m_axis;
    AbstractGraphData*  m_data;
    uint m_numElements;
};
