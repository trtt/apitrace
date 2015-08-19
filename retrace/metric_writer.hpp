#pragma once

#include <queue>
#include <string>
#include <unordered_map>

#include "metric_backend.hpp"

class ProfilerQuery
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

class ProfilerCall : public ProfilerQuery
{
public:
    struct data {
        bool isFrameEnd;
        unsigned no;
        unsigned program;
        const char* name;
    };

private:
    template<typename T>
    class StringTable
    {
    private:
        std::deque<std::string> strings;
        std::unordered_map<std::string, T> stringLookupTable;

    public:
        T getId(const std::string &str);
        std::string getString(T id);
    };

    static StringTable<int16_t> nameTable;

    bool isFrameEnd;
    unsigned no;
    unsigned program;
    int16_t nameTableEntry;

public:
    ProfilerCall(unsigned eventId, const data* queryData = nullptr);
    void writeHeader() const;
    void writeEntry() const;
};

class ProfilerDrawcall : public ProfilerCall
{
    ProfilerDrawcall(unsigned eventId, const data* queryData);
};

class ProfilerFrame : public ProfilerQuery
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
