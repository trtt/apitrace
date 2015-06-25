#include "metric_backend_amd_perfmon.hpp"

std::string Metric_AMD_perfmon::getName() {
    int length;
    std::string name;
    glGetPerfMonitorCounterStringAMD(getGroupId(), getId(), 0, &length, nullptr);
    name.resize(length);
    glGetPerfMonitorCounterStringAMD(getGroupId(), getId(), length, 0, &name[0]);
    return name;
}

GLenum Metric_AMD_perfmon::getSize() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(getGroupId(), getId(), GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return sizeof(GLuint);
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return sizeof(GLfloat);
    else if (type == GL_UNSIGNED_INT64_AMD) return sizeof(uint64_t);
    else return sizeof(GLuint);
}

MetricNumType Metric_AMD_perfmon::getNumType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(getGroupId(), getId(), GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return CNT_NUM_UINT;
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return CNT_NUM_FLOAT;
    else if (type == GL_UNSIGNED_INT64_AMD) return CNT_NUM_UINT64;
    else return CNT_NUM_UINT;
}

MetricType Metric_AMD_perfmon::getType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(getGroupId(), getId(), GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT || type == GL_UNSIGNED_INT64_AMD || type == GL_FLOAT) return CNT_TYPE_GENERIC;
    else if (type == GL_PERCENTAGE_AMD) return CNT_TYPE_PERCENT;
    else return CNT_TYPE_OTHER;
}

DataCollector::~DataCollector() {
    for (std::vector<unsigned*> t1 : data) {
        for (unsigned* t2 : t1) {
            delete[] t2;
        }
    }
}

unsigned* DataCollector::newDataBuffer(unsigned event, size_t size) {
    if (curEvent == 0) {
        std::vector<unsigned*> vec(1, new unsigned[size]);
        data.push_back(vec);
    } else {
        data[curPass].push_back(new unsigned[size]);
    }
    eventMap[event] = curEvent;
    return data[curPass][curEvent++];
}

void DataCollector::endPass() {
    curPass++;
    curEvent = 0;
}

unsigned* DataCollector::getDataBuffer(unsigned pass, unsigned event_) {
    if (eventMap.count(event_) > 0) {
        unsigned event = eventMap[event_];
        return data[pass][event];
    } else return nullptr;
}

unsigned DataCollector::getNumEvents() {
    return data[0].size();
}

unsigned DataCollector::getLastEvent() {
    return (curEvent - 1);
}


void MetricBackend_AMD_perfmon::enumGroups(enumGroupsCallback callback, void* userData) {
    std::vector<unsigned> groups;
    int num_groups;
    glGetPerfMonitorGroupsAMD(&num_groups, 0, nullptr);
    groups.resize(num_groups);
    glGetPerfMonitorGroupsAMD(nullptr, num_groups, &groups[0]);
    for(unsigned g : groups) {
        callback(g, userData);
    }
}

void MetricBackend_AMD_perfmon::enumMetrics(unsigned group, enumMetricsCallback callback, void* userData) {
    std::vector<unsigned> metrics;
    int num_metrics;
    Metric_AMD_perfmon metric(0,0);
    glGetPerfMonitorCountersAMD(group, &num_metrics, nullptr,  0, nullptr);
    metrics.resize(num_metrics);
    glGetPerfMonitorCountersAMD(group, nullptr, nullptr, num_metrics, &metrics[0]);
    for(unsigned c : metrics) {
        metric = Metric_AMD_perfmon(group, c);
        callback(&metric, userData);
    }
}

void MetricBackend_AMD_perfmon::enableMetric(Metric* metric_, bool perDraw) {
    Metric_AMD_perfmon metric(metric_->getGroupId(), metric_->getId());
    metrics.push_back(metric);
}

bool MetricBackend_AMD_perfmon::testMetrics(std::vector<Metric_AMD_perfmon>* metrics) {
    unsigned monitor;
    unsigned id;
    glGenPerfMonitorsAMD(1, &monitor);
    for (Metric_AMD_perfmon c : *metrics) {
        id = c.getId();
        glSelectPerfMonitorCountersAMD(monitor, 1, c.getGroupId(), 1, &id);
    }
    glGetError();
    glBeginPerfMonitorAMD(monitor);
    GLenum err = glGetError();
    glEndPerfMonitorAMD(monitor);
    glDeletePerfMonitorsAMD(1, &monitor);
    if (err == GL_INVALID_OPERATION) {
        return 0;
    }
    return 1;
}

unsigned MetricBackend_AMD_perfmon::generatePasses() {
    std::vector<Metric_AMD_perfmon> copyMetrics(metrics);
    std::vector<Metric_AMD_perfmon> newPass;
    while (!copyMetrics.empty()) {
        std::vector<Metric_AMD_perfmon>::iterator it = copyMetrics.begin();
        while (it != copyMetrics.end()) {
            newPass.push_back(*it);
            if (!testMetrics(&newPass)) {
                newPass.pop_back();
                break;
            }
            it = copyMetrics.erase(it);
        }
        passes.push_back(newPass);
        newPass.clear();
    }
    numPasses = passes.size();
    return numPasses;
}

void MetricBackend_AMD_perfmon::beginPass(bool perFrame_) {
    if (curPass == 0) {
        perFrame = perFrame_;
        generatePasses();
    }
    glGenPerfMonitorsAMD(NUM_MONITORS, monitors);
    unsigned id;
    for (Metric_AMD_perfmon c : passes[curPass]) {
        id = c.getId();
        for (int k = 0; k < NUM_MONITORS; k++) {
            glSelectPerfMonitorCountersAMD(monitors[k], 1, c.getGroupId(), 1, &id);
        }
    }
    curMonitor = 0;
    firstRound = 1;
    curEvent = 0;
}

void MetricBackend_AMD_perfmon::endPass() {
    for (int k = 0; k < NUM_MONITORS; k++) {
        freeMonitor(k);
    }
    glDeletePerfMonitorsAMD(NUM_MONITORS, monitors);
    curPass++;
    collector.endPass();
}

void MetricBackend_AMD_perfmon::beginQuery(bool isDraw) {
    if (!isDraw && !perFrame) return;
    if (!firstRound) freeMonitor(curMonitor);
    monitorEvent[curMonitor] = curEvent;
    glBeginPerfMonitorAMD(monitors[curMonitor]);
}

void MetricBackend_AMD_perfmon::endQuery(bool isDraw) {
    curEvent++;
    if (!isDraw && !perFrame) return;
    glEndPerfMonitorAMD(monitors[curMonitor]);
    curMonitor++;
    if (curMonitor == NUM_MONITORS) firstRound = 0;
    curMonitor %= NUM_MONITORS;
}

void MetricBackend_AMD_perfmon::freeMonitor(unsigned monitor_) {
    unsigned monitor = monitors[monitor_];
    glFlush();
    GLuint dataAvail = 0;
    while (!dataAvail) {
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(GLuint), &dataAvail, nullptr);
    }
    GLuint size;
    glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLuint), &size, nullptr);
    // collect data
    glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD, size, collector.newDataBuffer(monitorEvent[monitor_], size/sizeof(unsigned)), nullptr);
}

void MetricBackend_AMD_perfmon::enumDataQueryId(unsigned id, enumDataCallback callback, void* userData) {
    for (unsigned j = 0; j < numPasses; j++) {
        unsigned* buf = collector.getDataBuffer(j, id);
        unsigned offset = 0;
        for (int k = 0; k < passes[j].size(); k++) {
            if (buf) {
                Metric_AMD_perfmon metric(buf[offset], buf[offset+1]);
                offset += 2;
                callback(&metric, id, &buf[offset], userData);
                offset += metric.getSize() / sizeof(unsigned);
            } else { // No data buffer (in case event #id is not a draw call)
                offset += 2;
                callback(&passes[j][k], id, nullptr, userData);
                offset += passes[j][k].getSize() / sizeof(unsigned);
            }
        }
    }
}

void MetricBackend_AMD_perfmon::enumData(enumDataCallback callback, void* userData) {
    for (unsigned i = 0; i < curEvent; i++) {
        enumDataQueryId(i, callback, userData);
    }
}

unsigned MetricBackend_AMD_perfmon::getNumPasses() {
    return numPasses;
}

bool MetricBackend_AMD_perfmon::isLastPass() {
    return (numPasses-1 <= curPass);
}

unsigned MetricBackend_AMD_perfmon::getLastQueryId() {
    return (curEvent-1);
}

MetricBackend_AMD_perfmon& MetricBackend_AMD_perfmon::getInstance() {
    static MetricBackend_AMD_perfmon backend;
    return backend;
}
