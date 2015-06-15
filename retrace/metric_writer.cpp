#include <queue>
#include "trace_profiler.hpp"
#include "json.hpp"

struct ProfilerCall {
    unsigned no;

    unsigned program;

    int64_t gpuStart;
    int64_t gpuDuration;

    int64_t cpuStart;
    int64_t cpuDuration;

    int64_t vsizeStart;
    int64_t vsizeDuration;
    int64_t rssStart;
    int64_t rssDuration;

    int64_t pixels;

    std::string name;
    int amdPerfMonQueryId;
};

class MetricWriter
{
private:
    static JSONWriter json;
    std::queue<ProfilerCall> queue;
    Api_GL_AMD_performance_monitor* amdPerfMon;

public:
    MetricWriter(Api_GL_AMD_performance_monitor* _amdPerfMon) : amdPerfMon(_amdPerfMon) {}

    void addCall(unsigned no,
                 const char* name,
                 unsigned program,
                 int64_t pixels,
                 int64_t gpuStart, int64_t gpuDuration,
                 int64_t cpuStart, int64_t cpuDuration,
                 int64_t vsizeStart, int64_t vsizeDuration,
                 int64_t rssStart, int64_t rssDuration,
                 int amdPerfMonQueryId)
    {
        ProfilerCall tempCall = {no, program, gpuStart, gpuDuration, cpuStart, cpuDuration, vsizeStart, vsizeDuration, rssStart, rssDuration, pixels, std::string(name), amdPerfMonQueryId};
        queue.push(tempCall);
    }

    static void writeApiData(Counter* counter, int event, void* data) {
        json.beginMember(counter->getName());
        switch(counter->getNumType()) {
            case CNT_UINT: json.writeInt(*(reinterpret_cast<unsigned*>(data))); break;
            case CNT_FLOAT: json.writeFloat(*(reinterpret_cast<float*>(data))); break;
            case CNT_UINT64: json.writeInt(*(reinterpret_cast<uint64_t*>(data))); break;
        }
        json.endMember();
    }

    void writeCall() {
        ProfilerCall tempCall = queue.front();
        json.beginObject();

        json.beginMember("no");
        json.writeInt(tempCall.no);
        json.endMember();

        json.beginMember("program");
        json.writeInt(tempCall.program);
        json.endMember();

        json.beginMember("gpuStart");
        json.writeInt(tempCall.gpuStart);
        json.endMember();

        json.beginMember("gpuDuration");
        json.writeInt(tempCall.gpuDuration);
        json.endMember();

        json.beginMember("cpuStart");
        json.writeInt(tempCall.cpuStart);
        json.endMember();

        json.beginMember("cpuDuration");
        json.writeInt(tempCall.cpuDuration);
        json.endMember();

        json.beginMember("vsizeStart");
        json.writeInt(tempCall.vsizeStart);
        json.endMember();

        json.beginMember("vsizeDuration");
        json.writeInt(tempCall.vsizeDuration);
        json.endMember();

        json.beginMember("rssStart");
        json.writeInt(tempCall.rssStart);
        json.endMember();

        json.beginMember("rssDuration");
        json.writeInt(tempCall.rssDuration);
        json.endMember();

        json.beginMember("pixels");
        json.writeInt(tempCall.pixels);
        json.endMember();

        json.beginMember("name");
        json.writeString(tempCall.name);
        json.endMember();

        if (tempCall.amdPerfMonQueryId != -1) {
            json.beginMember("GL_AMD_performance_monitor");
            json.beginObject();
            amdPerfMon->enumDataQueryId(tempCall.amdPerfMonQueryId, &writeApiData);
            json.endObject();
            json.endMember();
        }

        json.endObject();
        queue.pop();
    }

    void writeAll() {
        json.beginMember("calls");
        json.beginArray();
        while (!queue.empty()) {
            writeCall();
        }
        json.endArray();
        json.endMember();
    }

};

JSONWriter MetricWriter::json(std::cout);
