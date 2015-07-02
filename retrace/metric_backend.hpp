#pragma once

#include <memory>
#include <string>

// Numeric metric formats
enum MetricNumType {
    CNT_NUM_UINT = 0,
    CNT_NUM_FLOAT,
    CNT_NUM_UINT64,
    CNT_NUM_DOUBLE,
    CNT_NUM_BOOL,
    CNT_NUM_INT64
};

// Type of data metric represents
enum MetricType {
    CNT_TYPE_GENERIC = 0,
    CNT_TYPE_NUM,
    CNT_TYPE_DURATION,
    CNT_TYPE_PERCENT,
    CNT_TYPE_TIMESTAMP,
    CNT_TYPE_OTHER
};

// Base class for metric:
class Metric
{
public:
    virtual ~Metric() {}

    virtual unsigned getId() = 0;

    virtual unsigned getGroupId() = 0;

    virtual std::string getName() = 0;

    virtual MetricNumType getNumType() = 0;

    virtual MetricType getType() = 0;
};

typedef void (*enumGroupsCallback)(unsigned group, int error, void* userData);
typedef void (*enumMetricsCallback)(Metric* metric, int error, void* userData);
typedef void (*enumDataCallback)(Metric* metric, int event, void* data, int error, void* userData);

// Base backend class:
class MetricBackend
{
public:
    virtual ~MetricBackend() {}

    virtual bool isSupported() = 0;
    /*
     * Some backend constructors require current gl context as an input argument
     * isSupported() needs to use it, therefore not static
    */

    virtual void enumGroups(enumGroupsCallback callback, void* userData = nullptr) = 0;

    virtual void enumMetrics(unsigned group, enumMetricsCallback callback, void* userData = nullptr) = 0;

    virtual std::string getGroupName(unsigned group) = 0;

    virtual std::unique_ptr<Metric> getMetricById(unsigned groupId, unsigned metricId);

    virtual std::unique_ptr<Metric> getMetricByName(std::string metricName);

    virtual int enableMetric(Metric* metric, bool perDraw = true) = 0;
    /*
     * returns integer - error code
     * 0 - no error
    */

    virtual void beginPass(bool perFrame = false) = 0;
    /* Passes are generated in the first beginPass()
     * based on metrics enabled via enableMetric().
     * This call must be used after context is initialized.
    */
    virtual void endPass() = 0;

    virtual void beginQuery(bool isDraw = false) = 0; // Query some unit (draw-call, frame)

    virtual void endQuery(bool isDraw = false) = 0;

    virtual void enumDataQueryId(unsigned id, enumDataCallback callback, void* userData = nullptr) = 0;

    virtual void enumData(enumDataCallback callback, void* userData = nullptr) = 0;

    virtual unsigned getNumPasses() = 0;

    virtual unsigned getLastQueryId() = 0;
};
