#pragma once

#include <string>

// Numeric counter formats
enum CounterNumType {
    CNT_NUM_UINT = 0,
    CNT_NUM_FLOAT,
    CNT_NUM_UINT64,
    CNT_NUM_DOUBLE,
    CNT_NUM_BOOL,
    CNT_NUM_INT64
};

// Type of data counter represents
enum CounterType {
    CNT_TYPE_GENERIC = 0,
    CNT_TYPE_NUM,
    CNT_TYPE_DURATION,
    CNT_TYPE_PERCENT,
    CNT_TYPE_TIMESTAMP,
    CNT_TYPE_OTHER
};

// Base class for counter:
class Counter
{
public:
    virtual ~Counter() {}

    virtual unsigned getId() = 0;

    virtual unsigned getGroupId() = 0;

    virtual std::string getName() = 0;

    virtual CounterNumType getNumType() = 0;

    virtual CounterType getType() = 0;
};

typedef void (*enumGroupsCallback)(unsigned group);
typedef void (*enumCountersCallback)(Counter* counter);
typedef void (*enumDataCallback)(Counter* counter, int event, void* data);

// Base backend class:
class Api_Base
{
public:
    virtual ~Api_Base() {}

    virtual void enumGroups(enumGroupsCallback callback) = 0;

    virtual void enumCounters(unsigned group, enumCountersCallback callback) = 0;

    virtual void enableCounter(Counter* counter, bool perDraw = true) = 0;

    virtual void beginPass(bool perFrame = false) = 0;
    /* Passes are generated in the first beginPass()
     * based on counters enabled via enableCounter().
     * This call must be used after context is initialized.
    */
    virtual void endPass() = 0;

    virtual void beginQuery(bool isDraw = false) = 0; // Query some unit (draw-call, frame)

    virtual void endQuery(bool isDraw = false) = 0;

    virtual void enumDataQueryId(unsigned id, enumDataCallback callback) = 0;

    virtual void enumData(enumDataCallback callback) = 0;

    virtual unsigned getNumPasses() = 0;

    virtual bool isLastPass() = 0;

    virtual unsigned getLastQueryId() = 0;
};
