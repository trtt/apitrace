#include <iostream>
#include "api_common.hpp"

unsigned Counter_common::getId() {
    return id;
}

unsigned Counter_common::getGroupId() {
    return group;
}

std::string Counter_common::getName() {
    return name;
}

CounterNumType Counter_common::getNumType() {
    return nType;
}

CounterType Counter_common::getType() {
    return type;
}


int64_t Counter_cpu::getCurrentTime() {
    GLint64 timestamp = 0;
    glGetInteger64v(GL_TIMESTAMP, &timestamp);
    return timestamp;
}

void Counter_cpu::setup() {
    double cpuTimeScale = 1.0E9 / 1000000000;
    baseTime = getCurrentTime() * cpuTimeScale;
}

void Counter_cpu::beginQuery() {
    if (start) data.push_back(getCurrentTime() - baseTime);
}

void Counter_cpu::endQuery() {
    if (!start) data.push_back(getCurrentTime() - baseTime);
}

void* Counter_cpu::getData(unsigned event) {
    return &data[event];
}


int64_t Counter_gpu::getCurrentTime() {
    GLint64 timestamp = 0;
    glGetInteger64v(GL_TIMESTAMP, &timestamp);
    return timestamp;
}

void Counter_gpu::setup() {
    double cpuTimeScale = 1.0E9 / 1000000000;
    baseTime = getCurrentTime() * cpuTimeScale;
}

void Counter_gpu::beginQuery() {
    glGenQueries(1, &query);
    if (start) {
        glQueryCounter(query, GL_TIMESTAMP);
    } else {
        glBeginQuery(GL_TIME_ELAPSED, query);
    }
}

void Counter_gpu::endQuery() {
    int64_t stamp;
    if (!start) glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjecti64v(query, GL_QUERY_RESULT, &stamp);
    if (start) stamp -= baseTime;
    data.push_back(stamp);
    glDeleteQueries(1, &query);
}

void* Counter_gpu::getData(unsigned event) {
    return &data[event];
}

Api_common::Api_common()
: curEvent(0), curDrawEvent(0),
cpuStart(1, 1, true),
cpuEnd(1, 2, false),
gpuStart(2, 1, true),
gpuDuration(2, 2, false)
{}

void Api_common::enumGroups(enumGroupsCallback callback) {
    callback(1);
    callback(2);
}

void Api_common::enumCounters(unsigned group, enumCountersCallback callback) {
    switch(group) {
    case 1:
        callback(&cpuStart);
        callback(&cpuEnd);
        break;
    case 2:
        callback(&gpuStart);
        callback(&gpuDuration);
        break;
    }
}

void Api_common::enableCounter(Counter* counter, bool perDraw) {
    switch(counter->getGroupId()) {
    case 1:
        switch(counter->getId()) {
        case 1:
            metrics.push_back(&cpuStart);
            cpuStart.perDraw = perDraw;
            break;
        case 2:
            metrics.push_back(&cpuEnd);
            cpuEnd.perDraw = perDraw;
            break;
        }
        break;
    case 2:
        switch(counter->getId()) {
        case 1:
            metrics.push_back(&gpuStart);
            gpuStart.perDraw = perDraw;
            break;
        case 2:
            metrics.push_back(&gpuDuration);
            gpuDuration.perDraw = perDraw;
            break;
        }
        break;
    }
}

void Api_common::beginPass(bool perFrame_) {
    for (Counter_common* c : metrics) {
        c->setup();
    }
}

void Api_common::endPass() {
}

void Api_common::beginQuery(bool isDraw) {
    if (isDraw) eventMap[curEvent] = curDrawEvent;
    for (Counter_common* c : metrics) {
        if (c->perDraw && isDraw) c->beginQuery();
        else if (!c->perDraw) c->beginQuery();
    }
}

void Api_common::endQuery(bool isDraw) {
    for (Counter_common* c : metrics) {
        if (c->perDraw && isDraw) c->endQuery();
        else if (!c->perDraw) c->endQuery();
    }
    curEvent++;
    if (isDraw) curDrawEvent++;
}

void Api_common::enumDataQueryId(unsigned id, enumDataCallback callback) {
    for (Counter_common* c : metrics) {
        if (c->perDraw) {
            if (eventMap.count(id) > 0) {
                callback(c, id, c->getData(eventMap[id]));
            } else {
                callback(c, id, nullptr);
            }
        } else {
            callback(c, id, c->getData(id));
        }
    }
}

void Api_common::enumData(enumDataCallback callback) {
    for (unsigned i = 0; i < curEvent; i++) {
        enumDataQueryId(i, callback);
    }
}

unsigned Api_common::getNumPasses() {
    return 1;
}

bool Api_common::isLastPass() {
    return true;
}

unsigned Api_common::getLastQueryId() {
    return (curEvent-1);
}
