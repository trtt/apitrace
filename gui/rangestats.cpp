#include "rangestats.h"

#include <iostream>
void RangeStats::collect() {
    BarGraph::range_iterator it(m_ptr, m_start, m_duration);
    int index = it.index();

    m_numEvents = 0;
    if (index != -1) {
        m_max = m_ptr->data()->dataY()->data[index];
        m_min = m_max;
    } else {
        m_max = 0;
        m_min = 0;
    }
    m_mean = 0;
    while (index != -1) {
        ++m_numEvents;
        float value = m_ptr->data()->dataY()->data[index];
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
