#include <queue>
#include <vector>
#include "trace_profiler.hpp"

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
    std::queue<ProfilerCall> queue;
    std::queue<ProfilerFrame> queueFrame;
    std::vector<Api_Base*>* metricApis;

public:
    MetricWriter(std::vector<Api_Base*>* _metricApis) : perFrame(false), metricApis(_metricApis) {}

    void addCall(int no,
                 const char* name,
                 unsigned program, unsigned eventId)
    {
        ProfilerCall tempCall = {no, program, name, eventId};
        queue.push(tempCall);
    }

    void addFrame(unsigned eventId)
    {
        if (!perFrame) perFrame = true;
        ProfilerFrame tempFrame = {eventId};
        queueFrame.push(tempFrame);
    }

    static void writeApiData(Counter* counter, int event, void* data) {
        if (!header) {
            std::cout << "\t" << counter->getName();
            return;
        }
        if (!data) {
            std::cout << "\t" << "-";
            return;
        }
        switch(counter->getNumType()) {
            case CNT_NUM_UINT: std::cout << "\t" << *(reinterpret_cast<unsigned*>(data)); break;
            case CNT_NUM_FLOAT: std::cout << "\t" << *(reinterpret_cast<float*>(data)); break;
            case CNT_NUM_UINT64: std::cout << "\t" << *(reinterpret_cast<uint64_t*>(data)); break;
            case CNT_NUM_INT64: std::cout << "\t" << *(reinterpret_cast<int64_t*>(data)); break;
        }
    }

    void writeCall() {
        ProfilerCall tempCall = queue.front();

        if (!header) {
            std::cout << "#\tcall no\tprogram\tname";
            for (Api_Base* a : *metricApis) {
                a->enumDataQueryId(tempCall.eventId, &writeApiData);
            }
            std::cout << std::endl;
            header = true;
            return;
        }

        if (tempCall.no == -1) {
            std::cout << "frame_end" << std::endl;
            queue.pop();
            return;
        }

        std::cout << "call"
            << "\t" << tempCall.no
            << "\t" << tempCall.program
            << "\t" << tempCall.name;

        for (Api_Base* a : *metricApis) {
            a->enumDataQueryId(tempCall.eventId, &writeApiData);
        }

        std::cout << std::endl;

        queue.pop();
    }

    void writeFrame() {
        ProfilerFrame tempFrame = queueFrame.front();

        if (!header) {
            std::cout << "#";
            for (Api_Base* a : *metricApis) {
                a->enumDataQueryId(tempFrame.eventId, &writeApiData);
            }
            std::cout << std::endl;
            header = true;
            return;
        }

        std::cout << "frame";

        for (Api_Base* a : *metricApis) {
            a->enumDataQueryId(tempFrame.eventId, &writeApiData);
        }

        std::cout << std::endl;

        queueFrame.pop();
    }

    void writeAll() {
        if (!perFrame) {
            while (!queue.empty()) {
                writeCall();
            }
        } else {
            while (!queueFrame.empty()) {
                writeFrame();
            }
        }
    }

};

bool MetricWriter::header = false;

