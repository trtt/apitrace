#pragma once

#include <vector>
#include <string>
#include <map>

#include "glproc.hpp"
#include "metric_backend.hpp"

class Metric_common : public Metric
{
private:
    unsigned group, id;
    std::string name;
    MetricNumType nType;
    MetricType type;

public:
    bool perDraw;

    Metric_common(unsigned g, unsigned i, MetricNumType nT, MetricType t, std::string n) : group(g), id(i), name(n), nType(nT), type(t) {}

    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    MetricNumType getNumType();

    MetricType getType();

    virtual void setup()=0;

    virtual void beginQuery()=0;

    virtual void endQuery()=0;

    virtual void* getData(unsigned event)=0;
};

class Metric_cpu : public Metric_common
{
private:
    bool start; // cpuStart or cpuEnd
    int64_t baseTime;
    std::vector<int64_t> data;

    int64_t getCurrentTime();

public:
    Metric_cpu(unsigned g, unsigned i, bool start_) : Metric_common(g, i, CNT_NUM_INT64, CNT_TYPE_TIMESTAMP, start_ ? "CPU Start":"CPU End"), start(start_) {}

    void setup();

    void beginQuery();

    void endQuery();

    void* getData(unsigned event);
};


class Metric_gpu : public Metric_common
{
private:
    GLuint query;
    bool start; // gpuStart or gpuEnd
    int64_t baseTime;
    std::vector<int64_t> data;

    int64_t getCurrentTime();

public:
    Metric_gpu(unsigned g, unsigned i, bool start_) : Metric_common(g, i, CNT_NUM_INT64, CNT_TYPE_TIMESTAMP, start_ ? "GPU Start":"GPU Duration"), start(start_) {}

    void setup();

    void beginQuery();

    void endQuery();

    void* getData(unsigned event);
};

class MetricBackend_common : public MetricBackend
{
private:
    unsigned curEvent, curDrawEvent;

    Metric_cpu cpuStart;
    Metric_cpu cpuEnd;
    Metric_gpu gpuStart;
    Metric_gpu gpuDuration;

    std::vector<Metric_common*> metrics; // store metrics selected for profiling
    std::map<unsigned, unsigned> eventMap;

public:
    MetricBackend_common();
    void enumGroups(enumGroupsCallback callback);

    void enumMetrics(unsigned group, enumMetricsCallback callback);

    void enableMetric(Metric* metric, bool perDraw = true);

    void beginPass(bool perFrame = false);

    void endPass();

    void beginQuery(bool isDraw = false);

    void endQuery(bool isDraw = false);

    void enumDataQueryId(unsigned id, enumDataCallback callback);

    void enumData(enumDataCallback callback);

    unsigned getNumPasses();

    bool isLastPass();

    unsigned getLastQueryId();
};

