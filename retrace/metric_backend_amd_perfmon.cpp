#include "metric_backend_amd_perfmon.hpp"

unsigned Metric_AMD_perfmon::getId() {
    return id;
}

unsigned Metric_AMD_perfmon::getGroupId() {
    return group;
}

std::string Metric_AMD_perfmon::getName() {
    int length;
    std::string name;
    glGetPerfMonitorCounterStringAMD(group, id, 0, &length, nullptr);
    name.resize(length);
    glGetPerfMonitorCounterStringAMD(group, id, length, 0, &name[0]);
    return name;
}

GLenum Metric_AMD_perfmon::getSize() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return sizeof(GLuint);
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return sizeof(GLfloat);
    else if (type == GL_UNSIGNED_INT64_AMD) return sizeof(uint64_t);
    else return sizeof(GLuint);
}

MetricNumType Metric_AMD_perfmon::getNumType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT) return CNT_NUM_UINT;
    else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return CNT_NUM_FLOAT;
    else if (type == GL_UNSIGNED_INT64_AMD) return CNT_NUM_UINT64;
    else return CNT_NUM_UINT;
}

MetricType Metric_AMD_perfmon::getType() {
    GLenum type;
    glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
    if (type == GL_UNSIGNED_INT || type == GL_UNSIGNED_INT64_AMD || type == GL_FLOAT) return CNT_TYPE_GENERIC;
    else if (type == GL_PERCENTAGE_AMD) return CNT_TYPE_PERCENT;
    else return CNT_TYPE_OTHER;
}

MetricBackend_AMD_perfmon::DataCollector::~DataCollector() {
    for (std::vector<unsigned*> t1 : data) {
        for (unsigned* t2 : t1) {
            delete[] t2;
        }
    }
}

unsigned* MetricBackend_AMD_perfmon::DataCollector::newDataBuffer(unsigned event, size_t size) {
    if (curEvent == 0) {
        std::vector<unsigned*> vec(1, new unsigned[size]);
        data.push_back(vec);
    } else {
        data[curPass].push_back(new unsigned[size]);
    }
    eventMap[event] = curEvent;
    return data[curPass][curEvent++];
}

void MetricBackend_AMD_perfmon::DataCollector::endPass() {
    curPass++;
    curEvent = 0;
}

unsigned* MetricBackend_AMD_perfmon::DataCollector::getDataBuffer(unsigned pass, unsigned event_) {
    if (eventMap.count(event_) > 0) {
        unsigned event = eventMap[event_];
        return data[pass][event];
    } else return nullptr;
}

MetricBackend_AMD_perfmon::MetricBackend_AMD_perfmon(glretrace::Context* context) : numPasses(1), curPass(0), curEvent(0) {
    if (context->hasExtension("GL_AMD_performance_monitor")) {
        supported = true;
    } else {
        supported = false;
    }
}

bool MetricBackend_AMD_perfmon::isSupported() {
    return supported;
}

void MetricBackend_AMD_perfmon::enumGroups(enumGroupsCallback callback, void* userData) {
    std::vector<unsigned> groups;
    int num_groups;
    glGetPerfMonitorGroupsAMD(&num_groups, 0, nullptr);
    groups.resize(num_groups);
    glGetPerfMonitorGroupsAMD(nullptr, num_groups, &groups[0]);
    for(unsigned g : groups) {
        callback(g, 0, userData);
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
        callback(&metric, 0, userData);
    }
}

std::unique_ptr<Metric>
MetricBackend_AMD_perfmon::getMetricById(unsigned groupId, unsigned metricId)
{
    std::unique_ptr<Metric> p(new Metric_AMD_perfmon(groupId, metricId));
    return p;
}

void MetricBackend_AMD_perfmon::populateLookupGroups(unsigned group,
                                                     int error,
                                                     void* userData)
{
    reinterpret_cast<MetricBackend_AMD_perfmon*>(userData)->enumMetrics(group, populateLookupMetrics);
}

void MetricBackend_AMD_perfmon::populateLookupMetrics(Metric* metric,
                                                      int error,
                                                      void* userData)
{
    nameLookup[metric->getName()] = std::make_pair(metric->getGroupId(),
                                                   metric->getId());
}

std::unique_ptr<Metric>
MetricBackend_AMD_perfmon::getMetricByName(std::string metricName)
{
    if (nameLookup.empty()) {
        enumGroups(populateLookupGroups, this);
    }
    if (nameLookup.count(metricName) > 0) {
        std::unique_ptr<Metric> p(new Metric_AMD_perfmon(nameLookup[metricName].first,
                                                         nameLookup[metricName].second));
        return p;
    }
    else return nullptr;
}

std::string MetricBackend_AMD_perfmon::getGroupName(unsigned group) {
    int length;
    std::string name;
    glGetPerfMonitorGroupStringAMD(group, 0, &length, nullptr);
    name.resize(length);
    glGetPerfMonitorGroupStringAMD(group, length, 0, &name[0]);
    return name;
}

int MetricBackend_AMD_perfmon::enableMetric(Metric* metric_, bool perDraw) {
    unsigned id = metric_->getId();
    unsigned gid = metric_->getGroupId();
    unsigned monitor;

    glGenPerfMonitorsAMD(1, &monitor);
    glGetError();
    glSelectPerfMonitorCountersAMD(monitor, 1, gid, 1, &id);
    GLenum err = glGetError();
    glDeletePerfMonitorsAMD(1, &monitor);
    if (err == GL_INVALID_VALUE) {
        return 1;
    }

    Metric_AMD_perfmon metric(gid, id);
    metrics.push_back(metric);
    return 0;
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
    nameLookup.clear(); // no need in it after all metrics are set up
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
    if (!numPasses) return;
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
    if (!numPasses) return;
    for (int k = 0; k < NUM_MONITORS; k++) {
        freeMonitor(k);
    }
    glDeletePerfMonitorsAMD(NUM_MONITORS, monitors);
    curPass++;
    collector.endPass();
}

void MetricBackend_AMD_perfmon::beginQuery(bool isDraw) {
    if (!numPasses) return;
    if (!isDraw && !perFrame) return;
    if (!firstRound) freeMonitor(curMonitor);
    monitorEvent[curMonitor] = curEvent;
    glBeginPerfMonitorAMD(monitors[curMonitor]);
}

void MetricBackend_AMD_perfmon::endQuery(bool isDraw) {
    if (!numPasses) return;
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
                callback(&metric, id, &buf[offset], 0, userData);
                offset += metric.getSize() / sizeof(unsigned);
            } else { // No data buffer (in case event #id is not a draw call)
                offset += 2;
                callback(&passes[j][k], id, nullptr, 0, userData);
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

MetricBackend_AMD_perfmon& MetricBackend_AMD_perfmon::getInstance(glretrace::Context* context) {
    static MetricBackend_AMD_perfmon backend(context);
    return backend;
}


std::map<std::string, std::pair<unsigned, unsigned>> MetricBackend_AMD_perfmon::nameLookup;
