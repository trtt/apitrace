#pragma once

#include <vector>
#include <string>
#include <map>

#include "glproc.hpp"
#include "api_base.hpp"

class Counter_common : public Counter
{
private:
    unsigned group, id;
    std::string name;
    CounterNumType nType;
    CounterType type;

public:
    bool perDraw;

    Counter_common(unsigned g, unsigned i, CounterNumType nT, CounterType t, std::string n) : group(g), id(i), name(n), nType(nT), type(t) {}

    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    CounterNumType getNumType();

    CounterType getType();

    virtual void setup()=0;

    virtual void beginQuery()=0;

    virtual void endQuery()=0;

    virtual void* getData(unsigned event)=0;
};

class Counter_cpu : public Counter_common
{
private:
    bool start; // cpuStart or cpuEnd
    int64_t baseTime;
    std::vector<int64_t> data;

    int64_t getCurrentTime();

public:
    Counter_cpu(unsigned g, unsigned i, bool start_) : Counter_common(g, i, CNT_NUM_INT64, CNT_TYPE_TIMESTAMP, start_ ? "CPU Start":"CPU End"), start(start_) {}

    void setup();

    void beginQuery();

    void endQuery();

    void* getData(unsigned event);
};


class Counter_gpu : public Counter_common
{
private:
    GLuint query;
    bool start; // gpuStart or gpuEnd
    int64_t baseTime;
    std::vector<int64_t> data;

    int64_t getCurrentTime();

public:
    Counter_gpu(unsigned g, unsigned i, bool start_) : Counter_common(g, i, CNT_NUM_INT64, CNT_TYPE_TIMESTAMP, start_ ? "GPU Start":"GPU Duration"), start(start_) {}

    void setup();

    void beginQuery();

    void endQuery();

    void* getData(unsigned event);
};

class Api_common : public Api_Base
{
private:
    unsigned curEvent, curDrawEvent;

    Counter_cpu cpuStart;
    Counter_cpu cpuEnd;
    Counter_gpu gpuStart;
    Counter_gpu gpuDuration;

    std::vector<Counter_common*> metrics; // store metrics selected for profiling
    std::map<unsigned, unsigned> eventMap;

public:
    Api_common();
    void enumGroups(enumGroupsCallback callback);

    void enumCounters(unsigned group, enumCountersCallback callback);

    void enableCounter(Counter* counter, bool perDraw = true);

    void beginPass(bool perFrame = false);

    void endPass();

    void beginQuery(bool isDraw = false);

    void endQuery(bool isDraw = false);

    void enumDataQueryId(unsigned id, enumDataCallback callback);

    void enumData(enumDataCallback callback);

    unsigned getNumPasses();

    bool isLastPass();

    unsigned getLastQueryId();
};

