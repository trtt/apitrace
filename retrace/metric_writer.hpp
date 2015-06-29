#pragma once

#include <queue>
#include <vector>
#include <string>

#include "metric_backend.hpp"

struct ProfilerCall {
    int no;
    unsigned program;
    std::string name;
    unsigned eventId;
};

struct ProfilerFrame {
    unsigned eventId;
};

class MetricWriter
{
private:
    bool perFrame;
    static bool header;
    std::queue<ProfilerCall> callQueue;
    std::queue<ProfilerFrame> frameQueue;
    std::vector<MetricBackend*>* metricApis;

public:
    MetricWriter(std::vector<MetricBackend*>* _metricApis) : perFrame(false),
                                                             metricApis(_metricApis) {}

    void addCall(int no,
                 const char* name,
                 unsigned program, unsigned eventId);

    void addFrame(unsigned eventId);

    static void writeApiData(Metric* metric, int event, void* data, int error,
                             void* userData);

    void writeCall();

    void writeFrame();

    void writeAll();
};
