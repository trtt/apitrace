#include "api_gl_amd_performance_monitor.hpp"

unsigned Counter_GL_AMD_performance_monitor::getId() {
    return id;
}

unsigned Counter_GL_AMD_performance_monitor::getGroupId() {
    return group;
}

std::string Counter_GL_AMD_performance_monitor::getName() {
    int length;
    std::string name;
    glGetPerfMonitorCounterStringAMD(group, id, 0, &length, nullptr);
    name.resize(length);
    glGetPerfMonitorCounterStringAMD(group, id, length, 0, &name[0]);
    return name;
}

GLenum Counter_GL_AMD_performance_monitor::getSize() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return sizeof(GLuint);
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return sizeof(GLfloat);
    else if (type == GL_UNSIGNED_INT64_AMD) return sizeof(uint64_t);
    else return sizeof(GLuint);
}

CounterNumType Counter_GL_AMD_performance_monitor::getNumType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return CNT_NUM_UINT;
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return CNT_NUM_FLOAT;
    else if (type == GL_UNSIGNED_INT64_AMD) return CNT_NUM_UINT64;
    else return CNT_NUM_UINT;
}

CounterType Counter_GL_AMD_performance_monitor::getType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
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


void Api_GL_AMD_performance_monitor::enumGroups(enumGroupsCallback callback) {
    std::vector<unsigned> groups;
    int num_groups;
    glGetPerfMonitorGroupsAMD(&num_groups, 0, nullptr);
    groups.resize(num_groups);
    glGetPerfMonitorGroupsAMD(nullptr, num_groups, &groups[0]);
    for(unsigned g : groups) {
        callback(g);
    }
}

void Api_GL_AMD_performance_monitor::enumCounters(unsigned group, enumCountersCallback callback) {
    std::vector<unsigned> counters;
    int num_counters;
    Counter_GL_AMD_performance_monitor counter(0,0);
    glGetPerfMonitorCountersAMD(group, &num_counters, nullptr,  0, nullptr);
    counters.resize(num_counters);
    glGetPerfMonitorCountersAMD(group, nullptr, nullptr, num_counters, &counters[0]);
    for(unsigned c : counters) {
        counter = Counter_GL_AMD_performance_monitor(group, c);
        callback(&counter);
    }
}

void Api_GL_AMD_performance_monitor::enableCounter(Counter* counter, bool perDraw) {
    Counter_GL_AMD_performance_monitor metric(counter->getGroupId(), counter->getId());
    metrics.push_back(metric);
}

bool Api_GL_AMD_performance_monitor::testCounters(std::vector<Counter_GL_AMD_performance_monitor>* counters) {
    unsigned monitor;
    unsigned id;
    glGenPerfMonitorsAMD(1, &monitor);
    for (Counter_GL_AMD_performance_monitor c : *counters) {
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

unsigned Api_GL_AMD_performance_monitor::generatePasses() {
    std::vector<Counter_GL_AMD_performance_monitor> copyMetrics(metrics);
    std::vector<Counter_GL_AMD_performance_monitor> newPass;
    while (!copyMetrics.empty()) {
        std::vector<Counter_GL_AMD_performance_monitor>::iterator it = copyMetrics.begin();
        while (it != copyMetrics.end()) {
            newPass.push_back(*it);
            if (!testCounters(&newPass)) {
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

void Api_GL_AMD_performance_monitor::beginPass(bool perFrame_) {
    if (curPass == 0) {
        perFrame = perFrame_;
        generatePasses();
    }
    glGenPerfMonitorsAMD(NUM_MONITORS, monitors);
    unsigned id;
    for (Counter_GL_AMD_performance_monitor c : passes[curPass]) {
        id = c.getId();
        for (int k = 0; k < NUM_MONITORS; k++) {
            glSelectPerfMonitorCountersAMD(monitors[k], 1, c.getGroupId(), 1, &id);
        }
    }
    curMonitor = 0;
    firstRound = 1;
    curEvent = 0;
}

void Api_GL_AMD_performance_monitor::endPass() {
    for (int k = 0; k < NUM_MONITORS; k++) {
        freeMonitor(k);
    }
    glDeletePerfMonitorsAMD(NUM_MONITORS, monitors);
    curPass++;
    collector.endPass();
}

void Api_GL_AMD_performance_monitor::beginQuery(bool isDraw) {
    if (!isDraw && !perFrame) return;
    if (!firstRound) freeMonitor(curMonitor);
    monitorEvent[curMonitor] = curEvent;
    glBeginPerfMonitorAMD(monitors[curMonitor]);
}

void Api_GL_AMD_performance_monitor::endQuery(bool isDraw) {
    curEvent++;
    if (!isDraw && !perFrame) return;
    glEndPerfMonitorAMD(monitors[curMonitor]);
    curMonitor++;
    if (curMonitor == NUM_MONITORS) firstRound = 0;
    curMonitor %= NUM_MONITORS;
}

void Api_GL_AMD_performance_monitor::freeMonitor(unsigned monitor_) {
    unsigned monitor = monitors[monitor_];
    glFlush();
    GLuint dataAvail = 0;
    while (!dataAvail) {
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(GLuint), &dataAvail, nullptr);
    }
    GLuint size;
    glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLuint), &size, nullptr);
    size /= sizeof(unsigned);
    // collect data
    glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD, size, collector.newDataBuffer(monitorEvent[monitor_], size), nullptr);
}

void Api_GL_AMD_performance_monitor::enumDataQueryId(unsigned id, enumDataCallback callback) {
    for (unsigned j = 0; j < numPasses; j++) {
        unsigned* buf = collector.getDataBuffer(j, id);
        unsigned offset = 0;
        for (int k = 0; k < passes[j].size(); k++) {
            if (buf) {
                Counter_GL_AMD_performance_monitor counter(buf[offset], buf[offset+1]);
                offset += 2;
                callback(&counter, id, &buf[offset]);
                offset += counter.getSize() / sizeof(unsigned);
            } else { // No data buffer (in case event #id is not a draw call)
                offset += 2;
                callback(&passes[j][k], id, nullptr);
                offset += passes[j][k].getSize() / sizeof(unsigned);
            }
        }
    }
}

void Api_GL_AMD_performance_monitor::enumData(enumDataCallback callback) {
    for (unsigned i = 0; i < curEvent; i++) {
        enumDataQueryId(i, callback);
    }
}

unsigned Api_GL_AMD_performance_monitor::getNumPasses() {
    return numPasses;
}

bool Api_GL_AMD_performance_monitor::isLastPass() {
    return (numPasses-1 <= curPass);
}

unsigned Api_GL_AMD_performance_monitor::getLastQueryId() {
    return (curEvent-1);
}
