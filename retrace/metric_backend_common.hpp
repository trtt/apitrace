#pragma once

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <array>

#include "glproc.hpp"
#include "metric_backend.hpp"
#include "glretrace.hpp"

class Metric_common : public Metric
{
private:
    unsigned gId, id;
    std::string name, desc;
    MetricNumType nType;
    MetricType type;

public:
    Metric_common(unsigned gId, unsigned id, const std::string &name,
                  const std::string &desc, MetricNumType nT, MetricType t);

    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    std::string getDescription();

    MetricNumType getNumType();

    MetricType getType();

    // should be set by backend
    bool available;
    bool profiled[QUERY_BOUNDARY_LIST_END]; // profiled in cur pass
    bool enabled[QUERY_BOUNDARY_LIST_END]; // enabled for profiling
};

class MetricBackend_common : public MetricBackend
{
private:
    // storage class
    class Storage
    {
    private:
        std::deque<int64_t> data[QUERY_BOUNDARY_LIST_END];

    public:
        void addData(QueryBoundary boundary, int64_t data);
        int64_t* getData(QueryBoundary boundary, unsigned eventId);
    };

    // indexes into metrics vector
    enum {
        METRIC_CPU_START = 0,
        METRIC_CPU_DURATION,
        METRIC_GPU_START,
        METRIC_GPU_DURATION,
        METRIC_GPU_PIXELS,
        METRIC_CPU_VSIZE_START,
        METRIC_CPU_VSIZE_DURATION,
        METRIC_CPU_RSS_START,
        METRIC_CPU_RSS_DURATION,
        METRIC_LIST_END
    };

    enum {
        QUERY_GPU_START = 0,
        QUERY_GPU_DURATION,
        QUERY_OCCLUSION,
        QUERY_LIST_END,
    };

    // lookup tables
    std::map<std::pair<unsigned,unsigned>, Metric_common*> idLookup;
    std::map<std::string, Metric_common*> nameLookup;

    GLint64 baseTime;
    double cpuTimeScale;
    bool supportsTimestamp, supportsElapsed, supportsOcclusion;
    bool glQueriesNeeded[QUERY_BOUNDARY_LIST_END];
    bool glQueriesNeededAnyBoundary;
    bool cpugpuSync;
    bool twoPasses; // profiling in two passes
    unsigned curPass;

    std::vector<Metric_common> metrics;
    std::unique_ptr<Storage> data[METRIC_LIST_END][QUERY_BOUNDARY_LIST_END];
    //std::map<unsigned, unsigned> eventMap;

    // Helper vars for metrics
    std::queue<std::array<GLuint, QUERY_LIST_END>> queries[QUERY_BOUNDARY_LIST_END];
    int64_t cpuStart[QUERY_BOUNDARY_LIST_END];
    int64_t cpuEnd[QUERY_BOUNDARY_LIST_END];
    int64_t vsizeStart[QUERY_BOUNDARY_LIST_END];
    int64_t vsizeEnd[QUERY_BOUNDARY_LIST_END];
    int64_t rssStart[QUERY_BOUNDARY_LIST_END];
    int64_t rssEnd[QUERY_BOUNDARY_LIST_END];

    MetricBackend_common(glretrace::Context* context);

    MetricBackend_common(MetricBackend_common const&) = delete;

    void operator=(MetricBackend_common const&)       = delete;

public:
    bool isSupported();

    void enumGroups(enumGroupsCallback callback, void* userData = nullptr);

    void enumMetrics(unsigned group, enumMetricsCallback callback, void* userData = nullptr);

    std::unique_ptr<Metric> getMetricById(unsigned groupId, unsigned metricId);

    std::unique_ptr<Metric> getMetricByName(std::string metricName);

    std::string getGroupName(unsigned group);

    int enableMetric(Metric* metric, QueryBoundary pollingRule = QUERY_BOUNDARY_DRAWCALL);

    unsigned generatePasses();

    void beginPass();

    void endPass();

    void beginQuery(QueryBoundary boundary = QUERY_BOUNDARY_DRAWCALL);

    void endQuery(QueryBoundary boundary = QUERY_BOUNDARY_DRAWCALL);

    void enumDataQueryId(unsigned id, enumDataCallback callback,
                         QueryBoundary boundary, void* userData = nullptr);

    void enumData(enumDataCallback callback,
                  QueryBoundary boundary, void* userData = nullptr);

    unsigned getNumPasses();

    unsigned getLastQueryId();

    static MetricBackend_common& getInstance(glretrace::Context* context);


private:
    int64_t getCurrentTime(void);

    int64_t getTimeFrequency(void);

    void processQueries();
};

