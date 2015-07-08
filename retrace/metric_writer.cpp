#include <iostream>

#include "metric_writer.hpp"

void MetricWriter::addCall(int no,
        const char* name,
        unsigned program, unsigned eventId, bool isDraw)
{
    ProfilerCall tempCall = {no, program, name, eventId};
    if (isDraw) drawCallQueue.push(tempCall);
    callQueue.push(tempCall);
}

void MetricWriter::addFrame(unsigned eventId)
{
    ProfilerFrame tempFrame = {eventId};
    frameQueue.push(tempFrame);
}

void MetricWriter::writeApiData(Metric* metric, int event, void* data, int error,
                                void* userData) {
    if (!header) {
        std::cout << "\t" << metric->getName();
        return;
    }
    if (error) {
        std::cout << "\t" << "#ERR" << error;
        return;
    }
    if (!data) {
        std::cout << "\t" << "-";
        return;
    }
    switch(metric->getNumType()) {
        case CNT_NUM_UINT: std::cout << "\t" << *(reinterpret_cast<unsigned*>(data)); break;
        case CNT_NUM_FLOAT: std::cout << "\t" << *(reinterpret_cast<float*>(data)); break;
        case CNT_NUM_DOUBLE: std::cout << "\t" << *(reinterpret_cast<double*>(data)); break;
        case CNT_NUM_BOOL: std::cout << "\t" << *(reinterpret_cast<bool*>(data)); break;
        case CNT_NUM_UINT64: std::cout << "\t" << *(reinterpret_cast<uint64_t*>(data)); break;
        case CNT_NUM_INT64: std::cout << "\t" << *(reinterpret_cast<int64_t*>(data)); break;
    }
}

void MetricWriter::writeCall(bool isDraw) {
    ProfilerCall tempCall;
    QueryBoundary qb;
    if (!isDraw) {
        tempCall = callQueue.front();
        qb = QUERY_BOUNDARY_CALL;
    } else {
        tempCall = drawCallQueue.front();
        qb = QUERY_BOUNDARY_DRAWCALL;
    }

    if (!header) {
        std::cout << "#\tcall no\tprogram\tname";
        for (auto &a : *metricApis) {
            a->enumDataQueryId(tempCall.eventId, &writeApiData, qb);
        }
        std::cout << std::endl;
        header = true;
        return;
    }

    if (!isDraw) callQueue.pop();
    else drawCallQueue.pop();

    if (tempCall.no == -1) {
        std::cout << "frame_end" << std::endl;
        return;
    }

    std::cout << "call"
        << "\t" << tempCall.no
        << "\t" << tempCall.program
        << "\t" << tempCall.name;

    for (auto &a : *metricApis) {
        a->enumDataQueryId(tempCall.eventId, &writeApiData, qb);
    }

    std::cout << std::endl;
}

void MetricWriter::writeFrame() {
    ProfilerFrame tempFrame = frameQueue.front();

    if (!header) {
        std::cout << "#";
        for (auto &a : *metricApis) {
            a->enumDataQueryId(tempFrame.eventId, &writeApiData, QUERY_BOUNDARY_FRAME);
        }
        std::cout << std::endl;
        header = true;
        return;
    }

    std::cout << "frame";

    for (auto &a : *metricApis) {
        a->enumDataQueryId(tempFrame.eventId, &writeApiData, QUERY_BOUNDARY_FRAME);
    }

    std::cout << std::endl;

    frameQueue.pop();
}

void MetricWriter::writeAll(QueryBoundary boundary) {
    switch(boundary) {
        case QUERY_BOUNDARY_FRAME:
        while (!frameQueue.empty()) {
            writeFrame();
        }
        break;

        case QUERY_BOUNDARY_DRAWCALL:
        while (!drawCallQueue.empty()) {
            writeCall(true);
        }
        break;

        case QUERY_BOUNDARY_CALL:
        while (!callQueue.empty()) {
            writeCall(false);
        }
        break;
    }
    std::cout << std::endl;
    header = false;
}

bool MetricWriter::header = false;

