#include <iostream>
#include "metric_backend_common.hpp"

unsigned Metric_common::getId() {
    return id;
}

unsigned Metric_common::getGroupId() {
    return group;
}

std::string Metric_common::getName() {
    return name;
}

MetricNumType Metric_common::getNumType() {
    return nType;
}

MetricType Metric_common::getType() {
    return type;
}


int64_t Metric_cpu::getCurrentTime() {
    GLint64 timestamp = 0;
    glGetInteger64v(GL_TIMESTAMP, &timestamp);
    return timestamp;
}

void Metric_cpu::setup() {
    double cpuTimeScale = 1.0E9 / 1000000000;
    baseTime = getCurrentTime() * cpuTimeScale;
}

void Metric_cpu::beginQuery() {
    if (start) data.push_back(getCurrentTime() - baseTime);
}

void Metric_cpu::endQuery() {
    if (!start) data.push_back(getCurrentTime() - baseTime);
}

void* Metric_cpu::getData(unsigned event) {
    return &data[event];
}


int64_t Metric_gpu::getCurrentTime() {
    GLint64 timestamp = 0;
    glGetInteger64v(GL_TIMESTAMP, &timestamp);
    return timestamp;
}

void Metric_gpu::setup() {
    double cpuTimeScale = 1.0E9 / 1000000000;
    baseTime = getCurrentTime() * cpuTimeScale;
}

void Metric_gpu::beginQuery() {
    glGenQueries(1, &query);
    if (start) {
        glQueryCounter(query, GL_TIMESTAMP);
    } else {
        glBeginQuery(GL_TIME_ELAPSED, query);
    }
}

void Metric_gpu::endQuery() {
    int64_t stamp;
    if (!start) glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjecti64v(query, GL_QUERY_RESULT, &stamp);
    if (start) stamp -= baseTime;
    data.push_back(stamp);
    glDeleteQueries(1, &query);
}

void* Metric_gpu::getData(unsigned event) {
    return &data[event];
}

MetricBackend_common::MetricBackend_common()
: curEvent(0), curDrawEvent(0),
cpuStart(1, 1, true),
cpuEnd(1, 2, false),
gpuStart(2, 1, true),
gpuDuration(2, 2, false)
{}

void MetricBackend_common::enumGroups(enumGroupsCallback callback) {
    callback(1);
    callback(2);
}

void MetricBackend_common::enumMetrics(unsigned group, enumMetricsCallback callback) {
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

void MetricBackend_common::enableMetric(Metric* metric, bool perDraw) {
    switch(metric->getGroupId()) {
    case 1:
        switch(metric->getId()) {
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
        switch(metric->getId()) {
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

void MetricBackend_common::beginPass(bool perFrame_) {
    for (Metric_common* c : metrics) {
        c->setup();
    }
}

void MetricBackend_common::endPass() {
}

void MetricBackend_common::beginQuery(bool isDraw) {
    if (isDraw) eventMap[curEvent] = curDrawEvent;
    for (Metric_common* c : metrics) {
        if (c->perDraw && isDraw) c->beginQuery();
        else if (!c->perDraw) c->beginQuery();
    }
}

void MetricBackend_common::endQuery(bool isDraw) {
    for (Metric_common* c : metrics) {
        if (c->perDraw && isDraw) c->endQuery();
        else if (!c->perDraw) c->endQuery();
    }
    curEvent++;
    if (isDraw) curDrawEvent++;
}

void MetricBackend_common::enumDataQueryId(unsigned id, enumDataCallback callback) {
    for (Metric_common* c : metrics) {
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

void MetricBackend_common::enumData(enumDataCallback callback) {
    for (unsigned i = 0; i < curEvent; i++) {
        enumDataQueryId(i, callback);
    }
}

unsigned MetricBackend_common::getNumPasses() {
    return 1;
}

bool MetricBackend_common::isLastPass() {
    return true;
}

unsigned MetricBackend_common::getLastQueryId() {
    return (curEvent-1);
}
