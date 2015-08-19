#include <iostream>

#include "metric_writer.hpp"

void ProfilerQuery::writeMetricHeader(Metric* metric, int event, void* data, int error,
                                      void* userData) {
    std::cout << "\t" << metric->name();
}

void ProfilerQuery::writeMetricEntry(Metric* metric, int event, void* data, int error,
                                     void* userData) {
    if (error) {
        std::cout << "\t" << "#ERR" << error;
        return;
    }
    if (!data) {
        std::cout << "\t" << "-";
        return;
    }
    switch(metric->numType()) {
        case CNT_NUM_UINT: std::cout << "\t" << *(reinterpret_cast<unsigned*>(data)); break;
        case CNT_NUM_FLOAT: std::cout << "\t" << *(reinterpret_cast<float*>(data)); break;
        case CNT_NUM_DOUBLE: std::cout << "\t" << *(reinterpret_cast<double*>(data)); break;
        case CNT_NUM_BOOL: std::cout << "\t" << *(reinterpret_cast<bool*>(data)); break;
        case CNT_NUM_UINT64: std::cout << "\t" << *(reinterpret_cast<uint64_t*>(data)); break;
        case CNT_NUM_INT64: std::cout << "\t" << *(reinterpret_cast<int64_t*>(data)); break;
    }
}

void ProfilerQuery::writeHeader() const {
    for (auto &a : *metricBackends) {
        a->enumDataQueryId(eventId, &writeMetricHeader, qb);
    }
    std::cout << std::endl;
}

void ProfilerQuery::writeEntry() const {
    for (auto &a : *metricBackends) {
        a->enumDataQueryId(eventId, &writeMetricEntry, qb);
    }
    std::cout << std::endl;
}

template<typename T>
T ProfilerCall::StringTable<T>::getId(const std::string &str) {
    auto res = stringLookupTable.find(str);
    T index;
    if (res == stringLookupTable.end()) {
        index = static_cast<T>(strings.size());
        strings.push_back(str);
        stringLookupTable[str] = index;
    } else {
        index = res->second;
    }
    return index;
}

template<typename T>
std::string ProfilerCall::StringTable<T>::getString(T id) {
    return strings[static_cast<typename decltype(stringLookupTable)::size_type>(id)];
}


ProfilerCall::ProfilerCall(unsigned eventId, const data* queryData)
    : ProfilerQuery(QUERY_BOUNDARY_CALL, eventId)
{
    if (queryData) {
        isFrameEnd = queryData->isFrameEnd;
        no = queryData->no;
        program = queryData->program;
        nameTableEntry = nameTable.getId(queryData->name);
    }
}


void ProfilerCall::writeHeader() const {
    std::cout << "#\tcall no\tprogram\tname";
    ProfilerQuery::writeHeader();
}

void ProfilerCall::writeEntry() const {
    if (isFrameEnd) {
        std::cout << "frame_end" << std::endl;
    } else {
        std::cout << "call"
            << "\t" << no
            << "\t" << program
            << "\t" << nameTable.getString(nameTableEntry);
        ProfilerQuery::writeEntry();
    }
}

void ProfilerFrame::writeHeader() const {
    std::cout << "#";
    ProfilerQuery::writeHeader();
}

void ProfilerFrame::writeEntry() const {
    std::cout << "frame";
    ProfilerQuery::writeEntry();
}

ProfilerDrawcall::ProfilerDrawcall(unsigned eventId, const data* queryData)
        : ProfilerCall( eventId, queryData)
{
    qb = QUERY_BOUNDARY_DRAWCALL;
}

MetricWriter::MetricWriter(std::vector<MetricBackend*> &metricBackends)
{
    ProfilerQuery::metricBackends = &metricBackends;
}

void MetricWriter::addQuery(QueryBoundary boundary, unsigned eventId,
                            const void* queryData)
{
    switch (boundary) {
        case QUERY_BOUNDARY_FRAME:
            queryQueue[boundary].push(std::unique_ptr<ProfilerQuery>(new ProfilerFrame(eventId)));
            break;
        case QUERY_BOUNDARY_CALL:
            queryQueue[boundary].push(std::unique_ptr<ProfilerQuery>(new ProfilerCall(eventId,
                        reinterpret_cast<const ProfilerCall::data*>(queryData))));
            break;
        case QUERY_BOUNDARY_DRAWCALL:
            queryQueue[boundary].push(std::unique_ptr<ProfilerQuery>(new ProfilerDrawcall(eventId,
                        reinterpret_cast<const ProfilerCall::data*>(queryData))));
            break;
        default:
            break;
    }
}

void MetricWriter::writeQuery(QueryBoundary boundary) {
    (queryQueue[boundary].front())->writeEntry();
    queryQueue[boundary].pop();
}

void MetricWriter::writeAll(QueryBoundary boundary) {
    (queryQueue[boundary].front())->writeHeader();
    while (!queryQueue[boundary].empty()) {
        writeQuery(boundary);
    }
    std::cout << std::endl;
}

std::vector<MetricBackend*>* ProfilerQuery::metricBackends = nullptr;

ProfilerCall::StringTable<int16_t> ProfilerCall::nameTable;
