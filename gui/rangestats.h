#include "bargraph.h"

#pragma once

class RangeStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(BarGraph* graph READ graph WRITE setGraph)
    Q_PROPERTY(qulonglong start MEMBER m_start)
    Q_PROPERTY(qulonglong duration MEMBER m_duration)
    Q_PROPERTY(float numEvents MEMBER m_numEvents NOTIFY numEventsChanged)
    Q_PROPERTY(float max MEMBER m_max NOTIFY maxChanged)
    Q_PROPERTY(float mean MEMBER m_mean NOTIFY meanChanged)
    Q_PROPERTY(float min MEMBER m_min NOTIFY minChanged)

public:
    RangeStats() : m_numEvents(0), m_max(0), m_mean(0), m_min(0) {}
    BarGraph* graph() const { return m_ptr; }
    void setGraph(BarGraph* ptr) { m_ptr = ptr; }

    Q_INVOKABLE void collect();

signals:
    void numEventsChanged();
    void maxChanged();
    void meanChanged();
    void minChanged();

private:
    BarGraph* m_ptr;
    uint64_t m_start;
    uint64_t m_duration;

    uint m_numEvents;
    float m_max;
    float m_mean;
    float m_min;
};

