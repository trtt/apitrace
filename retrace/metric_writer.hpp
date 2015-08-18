#pragma once

#include <queue>
#include <string>

#include "metric_backend.hpp"

struct ProfilerQuery
{
private:
    unsigned eventId;
    static void writeMetricHeader(Metric* metric, int event, void* data, int error,
                                  void* userData);
    static void writeMetricEntry(Metric* metric, int event, void* data, int error,
                                 void* userData);

protected:
    QueryBoundary qb;

public:
    static std::vector<MetricBackend*>* metricBackends;

    ProfilerQuery(QueryBoundary qb, unsigned eventId)
        : eventId(eventId), qb(qb) {};
    virtual void writeHeader() const;
    virtual void writeEntry() const;
};

struct ProfilerCall : public ProfilerQuery
{
public:
    struct data {
        int no;
        unsigned program;
        std::string name;
    };

private:
    data queryData;

public:
    ProfilerCall(unsigned eventId, const data* queryData = nullptr)
        : ProfilerQuery(QUERY_BOUNDARY_CALL, eventId), queryData(*queryData) {};
    void writeHeader() const;
    void writeEntry() const;
};

struct ProfilerDrawcall : public ProfilerCall
{
    ProfilerDrawcall(unsigned eventId, const data* queryData);
};

struct ProfilerFrame : public ProfilerQuery
{
public:
    ProfilerFrame(unsigned eventId)
        : ProfilerQuery(QUERY_BOUNDARY_FRAME, eventId) {};
    void writeHeader() const;
    void writeEntry() const;
};

class MetricWriter
{
private:
    static bool header;
    std::queue<std::unique_ptr<ProfilerQuery>> queryQueue[QUERY_BOUNDARY_LIST_END];

public:
    MetricWriter(std::vector<MetricBackend*> &metricBackends);

    void addQuery(QueryBoundary boundary, unsigned eventId,
                  const void* queryData = nullptr);

    void writeQuery(QueryBoundary boundary);

    void writeAll(QueryBoundary boundary);
};
