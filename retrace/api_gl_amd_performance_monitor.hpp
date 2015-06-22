#pragma once

#include <vector>
#include <map>
#include <string>

#include "glproc.hpp"
#include "api_base.hpp"

#define NUM_MONITORS 1 // number of used AMD_perfmon monitors, stick to one at first

class Counter_GL_AMD_performance_monitor : public Counter
{
private:
    unsigned group, id;

public:
    Counter_GL_AMD_performance_monitor(unsigned g, unsigned i) : group(g), id(i) {}

    GLenum getSize();


    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    CounterNumType getNumType();

    CounterType getType();
};


class DataCollector
{
private:
    std::vector<std::vector<unsigned*>> data;
    std::map<unsigned, unsigned> eventMap; // drawCallId <-> callId
    unsigned curPass;
    unsigned curEvent;

public:
    DataCollector() : curPass(0), curEvent(0) {}

    ~DataCollector();

    unsigned* newDataBuffer(unsigned event, size_t size);

    void endPass();

    unsigned* getDataBuffer(unsigned pass, unsigned event);

    unsigned getNumEvents();

    unsigned getLastEvent();
};

class Api_GL_AMD_performance_monitor : public Api_Base
{
private:
    unsigned monitors[NUM_MONITORS]; // For cycling, using 2 in current implementation
    unsigned curMonitor;
    bool firstRound, perFrame;
    std::vector<std::vector<Counter_GL_AMD_performance_monitor>> passes; // metric sets for each pass
    int numPasses;
    int curPass;
    unsigned curEvent; // Currently evaluated event
    unsigned monitorEvent[NUM_MONITORS]; // Event saved in monitor
    DataCollector collector;
    std::vector<Counter_GL_AMD_performance_monitor> metrics; // store metrics selected for profiling

    bool testCounters(std::vector<Counter_GL_AMD_performance_monitor>* counters); // test if given set of counters can be sampled in one pass

    unsigned generatePasses(); // called in first beginPass

    void freeMonitor(unsigned monitor); // collect metrics data from the monitor

public:
    Api_GL_AMD_performance_monitor() : numPasses(1), curPass(0), curEvent(0) {}

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

