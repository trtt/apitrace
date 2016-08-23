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

#include "rangestats.h"

#include <iostream>
void RangeStats::collect() {
    AbstractGraph::range_iterator it(m_ptr, m_start, m_duration);
    int index = it.index();

    m_numEvents = 0;
    while (index != -1) {
        ++m_numEvents;
        index = ++it;
    }
    emit numEventsChanged();
}


void RangeStatsMinMax::collect() {
    AbstractGraph::range_iterator it(m_ptr, m_start, m_duration);
    int index = it.index();

    m_numEvents = 0;
    if (index != -1) {
        m_max = static_cast<BarGraphData*>(m_ptr->data())->dataY()->data[index];
        m_min = m_max;
    } else {
        m_max = 0;
        m_min = 0;
    }
    m_mean = 0;
    while (index != -1) {
        ++m_numEvents;
        float value = static_cast<BarGraphData*>(m_ptr->data())->dataY()->data[index];
        if (value > m_max) m_max = value;
        if (value < m_max) m_min = value;
        m_mean += value;

        index = ++it;
    }
    if (m_numEvents) m_mean /= m_numEvents;
    emit numEventsChanged();
    emit maxChanged();
    emit meanChanged();
    emit minChanged();
}
