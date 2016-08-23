#include "bargraph.h"
#include "metric_data_model.hpp"

#pragma once

class RangeStats : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractGraph* graph READ graph WRITE setGraph)
    Q_PROPERTY(qulonglong start MEMBER m_start)
    Q_PROPERTY(qulonglong duration MEMBER m_duration)
    Q_PROPERTY(float numEvents MEMBER m_numEvents NOTIFY numEventsChanged)

public:
    RangeStats() : m_numEvents(0) {}
    AbstractGraph* graph() const { return m_ptr; }
    void setGraph(AbstractGraph* ptr) { m_ptr = ptr; }

    Q_INVOKABLE virtual void collect();

signals:
    void numEventsChanged();

protected:
    AbstractGraph* m_ptr;
    uint64_t m_start;
    uint64_t m_duration;

    uint m_numEvents;
};


class RangeStatsMinMax : public RangeStats
{
    Q_OBJECT
    Q_PROPERTY(float max MEMBER m_max NOTIFY maxChanged)
    Q_PROPERTY(float mean MEMBER m_mean NOTIFY meanChanged)
    Q_PROPERTY(float min MEMBER m_min NOTIFY minChanged)

public:
    RangeStatsMinMax() : RangeStats(), m_max(0), m_mean(0), m_min(0) {}

    virtual void collect();

signals:
    void maxChanged();
    void meanChanged();
    void minChanged();

private:
    float m_max;
    float m_mean;
    float m_min;
};


class TimelineHelper : public QObject
{
    Q_OBJECT

public:
    TimelineHelper(MetricCallDataModel* model) : m_data(model->calls()) {}

    Q_INVOKABLE unsigned no(unsigned index) const { return m_data.no(index); }
    Q_INVOKABLE unsigned program(unsigned index) const { return m_data.program(index); }
    Q_INVOKABLE QString name(unsigned index) const { return QString::fromStdString(m_data.name(index)); }
    Q_INVOKABLE unsigned frame(unsigned index) const { return m_data.frame(index); }

private:
    DrawcallStorage& m_data;
};
