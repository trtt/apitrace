#pragma once

#include <vector>
#include <map>
#include <string>

#include "glproc.hpp"
#include "metric_backend.hpp"

#define NUM_MONITORS 1 // number of used AMD_perfmon monitors, stick to one at first

class Metric_AMD_perfmon : public Metric
{
private:
    unsigned group, id;

public:
    Metric_AMD_perfmon(unsigned g, unsigned i) : group(g), id(i) {}

    GLenum getSize();


    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    MetricNumType getNumType();

    MetricType getType();
};


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

    unsigned getNumEvents();

    unsigned getLastEvent();
};

class MetricBackend_AMD_perfmon : public MetricBackend
{
private:
    unsigned monitors[NUM_MONITORS]; // For cycling, using 2 in current implementation
    unsigned curMonitor;
    bool firstRound, perFrame;
    std::vector<std::vector<Metric_AMD_perfmon>> passes; // metric sets for each pass
    int numPasses;
    int curPass;
    unsigned curEvent; // Currently evaluated event
    unsigned monitorEvent[NUM_MONITORS]; // Event saved in monitor
    DataCollector collector;
    std::vector<Metric_AMD_perfmon> metrics; // store metrics selected for profiling

    MetricBackend_AMD_perfmon() : numPasses(1), curPass(0), curEvent(0) {}

    MetricBackend_AMD_perfmon(MetricBackend_AMD_perfmon const&) = delete;

    void operator=(MetricBackend_AMD_perfmon const&)            = delete;

    bool testMetrics(std::vector<Metric_AMD_perfmon>* metrics); // test if given set of metrics can be sampled in one pass

    unsigned generatePasses(); // called in first beginPass

    void freeMonitor(unsigned monitor); // collect metrics data from the monitor

public:
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

    static MetricBackend_AMD_perfmon& getInstance();
};

