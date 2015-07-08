#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

#include "glproc.hpp"
#include "metric_backend.hpp"
#include "glretrace.hpp"

#define NUM_MONITORS 1 // number of used AMD_perfmon monitors, stick to one at first

class Metric_AMD_perfmon : public Metric
{
private:
    unsigned group, id;
    MetricNumType nType;
    bool precached;

    void precache();

public:
    Metric_AMD_perfmon(unsigned g, unsigned i) : group(g), id(i),
                                                 nType(CNT_NUM_UINT),
                                                 precached(false) {}

    GLenum getSize();


    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    std::string getDescription();

    MetricNumType getNumType();

    MetricType getType();
};


class MetricBackend_AMD_perfmon : public MetricBackend
{
private:
    class DataCollector
    {
        private:
            std::vector<std::vector<unsigned*>> data;
            std::map<unsigned, unsigned> eventMap; // drawCallId <-> callId
            unsigned curPass;
            unsigned curEvent;

        public:
            DataCollector() : curPass(0), curEvent(0) {}

            ~DataCollector();

            unsigned* newDataBuffer(unsigned event, size_t size);

            void endPass();

            unsigned* getDataBuffer(unsigned pass, unsigned event);
    };

private:
    bool supported;
    unsigned monitors[NUM_MONITORS]; // For cycling, using 2 in current implementation
    unsigned curMonitor;
    bool firstRound, perFrame;
    std::vector<std::vector<Metric_AMD_perfmon>> passes; // metric sets for each pass
    /* metricOffsets[pass][Metric*] -- metric offset in data returned after profiling */
    std::vector<std::map<Metric_AMD_perfmon*, unsigned>> metricOffsets;
    int numPasses;
    int numFramePasses;
    int curPass;
    unsigned curEvent; // Currently evaluated event
    unsigned monitorEvent[NUM_MONITORS]; // Event saved in monitor
    DataCollector collector;
    std::vector<Metric_AMD_perfmon> metrics[2]; // store metrics selected for profiling
    static std::map<std::string, std::pair<unsigned, unsigned>> nameLookup;

    MetricBackend_AMD_perfmon(glretrace::Context* context);

    MetricBackend_AMD_perfmon(MetricBackend_AMD_perfmon const&) = delete;

    void operator=(MetricBackend_AMD_perfmon const&)            = delete;

    // test if given set of metrics can be sampled in one pass
    bool testMetrics(std::vector<Metric_AMD_perfmon>* metrics);

    void freeMonitor(unsigned monitor); // collect metrics data from the monitor

    static void populateLookupGroups(unsigned group, int error, void* userData);

    static void populateLookupMetrics(Metric* metric, int error, void* userData);

    void generatePassesBoundary(QueryBoundary boundary);

public:
    bool isSupported();

    void enumGroups(enumGroupsCallback callback, void* userData = nullptr);

    void enumMetrics(unsigned group, enumMetricsCallback callback,
                     void* userData = nullptr);

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
                         QueryBoundary boundary,
                         void* userData = nullptr);

    void enumData(enumDataCallback callback, QueryBoundary boundary,
                  void* userData = nullptr);

    unsigned getNumPasses();

    unsigned getLastQueryId();

    static MetricBackend_AMD_perfmon& getInstance(glretrace::Context* context);
};

