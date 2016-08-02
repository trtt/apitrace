#include "abstractgraph.h"

#include <QQuickWindow>

#include <cmath>


void TimelineAxis::acquireResHandle() {
    if (!m_numHandles) {
        xH->init();
        xL->init();
        xW->init();
    }
    m_numHandles++;
}

void TimelineAxis::freeResHandle() {
    if (!(--m_numHandles)) {
        if (xH.unique())
            xH->deinit();
        if (xL.unique())
            xL->deinit();
        if (xW.unique())
            xW->deinit();
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


void AbstractGraphData::acquireResHandle() {
    if (!m_numHandles) {
        m_dataFilter->init();
    }
    m_numHandles++;
}

void AbstractGraphData::freeResHandle() {
    if (!(--m_numHandles)) {
        if (m_dataFilter.unique())
            m_dataFilter->deinit();
    }
}


AbstractGraph::AbstractGraph(QQuickItem *parent)
    : QQuickFramebufferObject(parent),  m_renderer(0), m_filtered(true),
      m_needsUpdating(true), m_filter(0), m_bgcolor("white"),
      m_axis(nullptr), m_data(nullptr), m_numElements(0)
{
    setMirrorVertically(true);
}

void AbstractGraph::forceupdate() {
    m_needsUpdating = true;
    update();
}

void AbstractGraph::setAxis(TimelineAxis* axis) {
    m_axis = axis;
    connect(m_axis, &TimelineAxis::dispStartTimeChanged, this, &AbstractGraph::forceupdate);
    connect(m_axis, &TimelineAxis::dispEndTimeChanged, this, &AbstractGraph::forceupdate);
    connect(this, &AbstractGraph::numElementsChanged, this, &AbstractGraph::forceupdate);
}

bool AbstractGraph::isEventFiltered(int id) const {
    return (m_filtered && m_data->dataFilter()->data[id] != m_filter);
}

uint AbstractGraph::eventFilter(int id) const {
    return (m_filtered ? m_filter : m_data->dataFilter()->data[id]);
}

AbstractGraph::range_iterator::range_iterator(AbstractGraph* bg, uint64_t begin,
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

int AbstractGraph::range_iterator::operator++() {
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
    } while (ptr->m_filtered && ptr->m_data->dataFilter()->data[curElement] != ptr->m_filter);
    return curElement;
}


AbstractGraphRenderer::AbstractGraphRenderer(AbstractGraph* item)
    : m_item(item),
      m_axisCopy(nullptr), m_dataCopy(nullptr), m_numElementsCopy(0),
      m_filterCopy(0), m_bgcolorCopy("white")
{

}

AbstractGraphRenderer::~AbstractGraphRenderer() {
    m_axisCopy->freeResHandle();
    m_dataCopy->freeResHandle();
}

void AbstractGraphRenderer::synchronize(QQuickFramebufferObject* item) {
    AbstractGraph* i = static_cast<AbstractGraph*>(item);
    if (!i->m_needsUpdating || !i->m_axis || !i->m_data) return;

    // Prepare texture buffers if needed
    if (m_axisCopy != i->m_axis) {
        if (m_axisCopy) m_axisCopy->freeResHandle();
        m_axisCopy = i->m_axis;
        m_axisCopy->acquireResHandle();
    }
    if (m_dataCopy != i->m_data) {
        if (m_dataCopy) m_dataCopy->freeResHandle();
        m_dataCopy = i->m_data;
        m_dataCopy->acquireResHandle();
    }

    m_win = i->window();
    QPointF sceneCoord = i->parentItem()->mapToScene(QPointF(i->x(), i->y()));
    m_width = i->width();
    m_height = i->height();
    int winHeight = i->window()->size().height();

    if ((sceneCoord.y() + i->height() < 0) ||
        (sceneCoord.y() > winHeight))
    {
        i->m_offscreen = true;
    } else {
        i->m_offscreen = false;
    }

    // Not shown on screen = no reason to update
    if (i->m_offscreen) return;

    m_needsUpdating = true;

    m_filteredCopy = i->m_filtered;
    m_numElementsCopy = i->m_numElements;
    m_filterCopy = i->m_filter;
    m_bgcolorCopy = i->m_bgcolor;

    m_dispStartTimeCopy = i->m_axis->dispStartTime();
    m_dispEndTimeCopy = i->m_axis->dispEndTime();
    m_dispFirstEventCopy = i->m_axis->m_dispFirstEvent;
    m_dispLastEventCopy = i->m_axis->m_dispLastEvent;

    synchronizeAfter(item);
    i->m_needsUpdating = false;
}
