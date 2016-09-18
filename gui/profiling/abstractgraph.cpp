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

#include "abstractgraph.h"

#include <QQuickWindow>
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
#include <QSGSimpleTextureNode>
#endif

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
    m_dispFirstEvent = std::max(upper_bound64(m_firstEvent, m_lastEvent,
                                         xH->data, xL->data,
                                         1, m_dispStartTime) - 1, 0);
    m_dispLastEvent  = std::max(upper_bound64(m_firstEvent, m_lastEvent,
                                         xH->data, xL->data,
                                         1, m_dispEndTime) - 1, 0);
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
        if (m_dataFilter)
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
      m_filter(0), m_bgcolor("white"),
      m_axis(nullptr), m_data(nullptr), m_numElements(0)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    setMirrorVertically(true);
#endif
    connect(this, &AbstractGraph::numElementsChanged, this, &AbstractGraph::forceupdate);
    connect(this, &QQuickItem::heightChanged, this, &AbstractGraph::forceupdate);
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
QSGNode* AbstractGraph::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
{
    if (!node) {
        node = QQuickFramebufferObject::updatePaintNode(node, nodeData);
        QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode *>(node);
        if (n)
            n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        return node;
    }
    return QQuickFramebufferObject::updatePaintNode(node, nodeData);
}
#endif

void AbstractGraph::forceupdate() {
    m_needsUpdating = true;
    update();
}

void AbstractGraph::setAxis(TimelineAxis* axis) {
    if (axis) {
        m_axis = axis;
        connect(m_axis, &TimelineAxis::dispStartTimeChanged, this, &AbstractGraph::forceupdate);
        connect(m_axis, &TimelineAxis::dispEndTimeChanged, this, &AbstractGraph::forceupdate);
        emit axisChanged();
    }
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
	curElement = std::max(curElement, 0);
    if ( ((uint64_t)ptr->m_axis->xH->data[curElement] << 32) \
         + ptr->m_axis->xL->data[curElement] < begin) curElement = -1;
}

int AbstractGraph::range_iterator::operator++() {
    if (curElement == -1) return -1;
    do {
		if (curElement == ptr->m_axis->m_dispLastEvent) {
			curElement = -1;
			break;
		}
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
      m_axisCopy(item->m_axis), m_dataCopy(item->m_data),
      m_win(item->window())
{
    if (m_axisCopy) {
        m_axisCopy->acquireResHandle();
    }
    if (m_dataCopy) {
        m_dataCopy->acquireResHandle();
    }
    item->m_needsUpdating = true;
}

AbstractGraphRenderer::~AbstractGraphRenderer() {
    m_axisCopy->freeResHandle();
    m_dataCopy->freeResHandle();
}

void AbstractGraphRenderer::synchronize(QQuickFramebufferObject* item) {
    AbstractGraph* i = static_cast<AbstractGraph*>(item);
    if (i->m_ignoreUpdates) return;
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

    m_width = i->width();
    m_height = i->height();

    m_filteredCopy = i->m_filtered;
    m_numElementsCopy = i->m_numElements;
    m_filterCopy = i->m_filter;
    m_bgcolorCopy = i->m_bgcolor;

    m_dispStartTimeCopy = i->m_axis->dispStartTime();
    m_dispEndTimeCopy = i->m_axis->dispEndTime();
    m_dispFirstEventCopy = i->m_axis->m_dispFirstEvent;
    m_dispLastEventCopy = i->m_axis->m_dispLastEvent;

    synchronizeAfter(item);
    m_needsUpdating = true;
    i->m_needsUpdating = false;
}
