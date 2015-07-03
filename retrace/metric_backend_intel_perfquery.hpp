#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

#include "glproc.hpp"
#include "metric_backend.hpp"
#include "glretrace.hpp"

#define INTEL_NAME_LENGTH 256 // metric name with max 256 chars
#define INTEL_DESC_LENGTH 1024 // description max 1024 chars

class Metric_INTEL_perfquery : public Metric
{
private:
    unsigned group, id;

public:
    Metric_INTEL_perfquery(unsigned g, unsigned i) : group(g), id(i) {}

    unsigned getOffset();


    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    std::string getDescription();

    MetricNumType getNumType();

    MetricType getType();
};


class MetricBackend_INTEL_perfquery : public MetricBackend
{
private:
    class DataCollector
    {
        private:
            std::vector<std::vector<unsigned char*>> data;
            std::map<unsigned, unsigned> eventMap; // drawCallId <-> callId
            unsigned curPass;
            unsigned curEvent;

        public:
            DataCollector() : curPass(0), curEvent(0) {}

            ~DataCollector();

            unsigned char* newDataBuffer(unsigned event, size_t size);

            void endPass();

            unsigned char* getDataBuffer(unsigned pass, unsigned event);
    };

private:
    std::map<unsigned, std::vector<Metric_INTEL_perfquery>> passes; // map from query id to its Metric list
    /* curQueryMetrics -- iterator through passes */
    std::map<unsigned, std::vector<Metric_INTEL_perfquery>>::iterator curQueryMetrics;
    unsigned curQuery;
    bool supported;
    bool perFrame;
    int numPasses;
    int curPass;
    unsigned curEvent; // Currently evaluated event
    DataCollector collector;
    /* nameLookup for querying metrics by name */
    static std::map<std::string, std::pair<unsigned, unsigned>> nameLookup;

    MetricBackend_INTEL_perfquery(glretrace::Context* context);

    MetricBackend_INTEL_perfquery(MetricBackend_INTEL_perfquery const&) = delete;

    void operator=(MetricBackend_INTEL_perfquery const&)            = delete;

    void freeQuery(unsigned event); // collect metrics data from the query

    static void populateLookupGroups(unsigned group, int error, void* userData);

    static void populateLookupMetrics(Metric* metric, int error, void* userData);

public:
    bool isSupported();

    void enumGroups(enumGroupsCallback callback, void* userData = nullptr);

    void enumMetrics(unsigned group, enumMetricsCallback callback, void* userData = nullptr);

    std::unique_ptr<Metric> getMetricById(unsigned groupId, unsigned metricId);

    std::unique_ptr<Metric> getMetricByName(std::string metricName);

    std::string getGroupName(unsigned group);

    int enableMetric(Metric* metric, bool perDraw = true);

    void beginPass(bool perFrame = false);

    void endPass();

    void beginQuery(bool isDraw = false);

    void endQuery(bool isDraw = false);

    void enumDataQueryId(unsigned id, enumDataCallback callback, void* userData = nullptr);

    void enumData(enumDataCallback callback, void* userData = nullptr);

    unsigned getNumPasses();

    unsigned getLastQueryId();

    static MetricBackend_INTEL_perfquery& getInstance(glretrace::Context* context);
};

