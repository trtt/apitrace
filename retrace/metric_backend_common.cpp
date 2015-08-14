#include "metric_backend_common.hpp"
#include "os_time.hpp"
#include "os_memory.hpp"

void
MetricBackend_common::Storage::addData(QueryBoundary boundary, int64_t data) {
    this->data[boundary].push_back(data);
}

int64_t* MetricBackend_common::Storage::getData(QueryBoundary boundary,
                                                 unsigned eventId)
{
    return &(data[boundary][eventId]);
}

Metric_common::Metric_common(unsigned gId, unsigned id, const std::string &name,
                             const std::string &desc, MetricNumType nT, MetricType t)
    : gId(gId), id(id), name(name), desc(desc), nType(nT), type(t), available(false)
{
    for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
        profiled[i] = false;
        enabled[i] = false;
    }
}

unsigned Metric_common::getId() {
    return id;
}

unsigned Metric_common::getGroupId() {
    return gId;
}

std::string Metric_common::getName() {
    return name;
}

std::string Metric_common::getDescription() {
    return desc;
}

MetricNumType Metric_common::getNumType() {
    return nType;
}

MetricType Metric_common::getType() {
    return type;
}

MetricBackend_common::MetricBackend_common(glretrace::Context* context)
{
    glprofile::Profile currentProfile = context->profile();
    supportsTimestamp   = currentProfile.versionGreaterOrEqual(glprofile::API_GL, 3, 3) ||
                          context->hasExtension("GL_ARB_timer_query");
    supportsElapsed     = context->hasExtension("GL_EXT_timer_query") || supportsTimestamp;
    supportsOcclusion   = currentProfile.versionGreaterOrEqual(glprofile::API_GL, 1, 5);

    #ifdef __APPLE__
        // GL_TIMESTAMP doesn't work on Apple.  GL_TIME_ELAPSED still does however.
        // http://lists.apple.com/archives/mac-opengl/2014/Nov/threads.html#00001
        supportsTimestamp   = false;
    #endif

    // Add metrics below
    metrics.emplace_back(0, 0, "CPU Start", "", CNT_NUM_INT64, CNT_TYPE_TIMESTAMP);
    metrics.emplace_back(0, 1, "CPU Duration", "", CNT_NUM_INT64, CNT_TYPE_DURATION);
    metrics.emplace_back(1, 0, "GPU Start", "", CNT_NUM_INT64, CNT_TYPE_TIMESTAMP);
    metrics.emplace_back(1, 1, "GPU Duration", "", CNT_NUM_INT64, CNT_TYPE_DURATION);
    metrics.emplace_back(1, 2, "Pixels Drawn", "", CNT_NUM_INT64, CNT_TYPE_GENERIC);
    metrics.emplace_back(0, 2, "VSIZE Start", "", CNT_NUM_INT64, CNT_TYPE_GENERIC);
    metrics.emplace_back(0, 3, "VSIZE Duration", "", CNT_NUM_INT64, CNT_TYPE_GENERIC);
    metrics.emplace_back(0, 4, "RSS Start", "", CNT_NUM_INT64, CNT_TYPE_GENERIC);
    metrics.emplace_back(0, 5, "RSS Duration", "", CNT_NUM_INT64, CNT_TYPE_GENERIC);

    metrics[METRIC_CPU_START].available = true;
    metrics[METRIC_CPU_DURATION].available = true;
    metrics[METRIC_CPU_VSIZE_START].available = true;
    metrics[METRIC_CPU_VSIZE_DURATION].available = true;
    metrics[METRIC_CPU_RSS_START].available = true;
    metrics[METRIC_CPU_RSS_DURATION].available = true;
    if (supportsTimestamp) metrics[METRIC_GPU_START].available = true;
    if (supportsElapsed) {
        GLint bits = 0;
        glGetQueryiv(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS, &bits);
        if (bits) metrics[METRIC_GPU_DURATION].available = true;
    }
    if (supportsOcclusion) {
        metrics[METRIC_GPU_PIXELS].available = true;
    }

    // populate lookups
    for (auto &m : metrics) {
        idLookup[std::make_pair(m.getGroupId(), m.getId())] = &m;
        nameLookup[m.getName()] = &m;
    }
}

int64_t MetricBackend_common::getCurrentTime(void) {
    if (supportsTimestamp && cpugpuSync) {
        /* Get the current GL time without stalling */
        GLint64 timestamp = 0;
        glGetInteger64v(GL_TIMESTAMP, &timestamp);
        return timestamp;
    } else {
        return os::getTime();
    }
}

int64_t MetricBackend_common::getTimeFrequency(void) {
    if (supportsTimestamp && cpugpuSync) {
        return 1000000000;
    } else {
        return os::timeFrequency;
    }
}


bool MetricBackend_common::isSupported() {
    return true;
    // though individual metrics might be not supported
}

void MetricBackend_common::enumGroups(enumGroupsCallback callback, void* userData) {
    callback(0, 0, userData); // cpu group
    callback(1, 0, userData); // gpu group
}

std::string MetricBackend_common::getGroupName(unsigned group) {
    switch(group) {
        case 0:
            return "CPU";
        case 1:
            return "GPU";
        default:
            return "";
    }
}

void MetricBackend_common::enumMetrics(unsigned group, enumMetricsCallback callback, void* userData) {
    for (auto &m : metrics) {
        if (m.getGroupId() == group && m.available) {
            callback(&m, 0, userData);
        }
    }
}

std::unique_ptr<Metric>
MetricBackend_common::getMetricById(unsigned groupId, unsigned metricId) {
    auto entryToCopy = idLookup.find(std::make_pair(groupId, metricId));
    if (entryToCopy != idLookup.end()) {
        return std::unique_ptr<Metric>(new Metric_common(*entryToCopy->second));
    } else {
        return nullptr;
    }
}

std::unique_ptr<Metric>
MetricBackend_common::getMetricByName(std::string metricName) {
    auto entryToCopy = nameLookup.find(metricName);
    if (entryToCopy != nameLookup.end()) {
        return std::unique_ptr<Metric>(new Metric_common(*entryToCopy->second));
    } else {
        return nullptr;
    }
}


int MetricBackend_common::enableMetric(Metric* metric, QueryBoundary pollingRule) {
    // metric is not necessarily the same object as in metrics[]
    auto entry = idLookup.find(std::make_pair(metric->getGroupId(), metric->getId()));
    if ((entry != idLookup.end()) && entry->second->available) {
        entry->second->enabled[pollingRule] = true;
        return 0;
    } else {
        return 1;
    }
}

unsigned MetricBackend_common::generatePasses() {
    // draw calls profiling not needed if all calls are profiled
    for (int i = 0; i < METRIC_LIST_END; i++) {
        if (metrics[i].enabled[QUERY_BOUNDARY_CALL]) {
            metrics[i].enabled[QUERY_BOUNDARY_DRAWCALL] = false;
        }
    }
    // setup storage for profiled metrics
    for (int i = 0; i < METRIC_LIST_END; i++) {
        for (int j = 0; j < QUERY_BOUNDARY_LIST_END; j++) {
            if (metrics[i].enabled[j]) {
                data[i][j] = std::unique_ptr<Storage>(new Storage);
            }
        }
    }
    // check if GL queries are needed
    glQueriesNeededAnyBoundary = false;
    for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
        if (metrics[METRIC_GPU_START].enabled[i] ||
            metrics[METRIC_GPU_DURATION].enabled[i] ||
            metrics[METRIC_GPU_PIXELS].enabled[i])
        {
            glQueriesNeeded[i] = true;
            glQueriesNeededAnyBoundary = true;
        } else {
            glQueriesNeeded[i] = false;
        }
    }
    // check if CPU <-> GPU sync is required
    // this is the case if any gpu time is requested
    cpugpuSync = false;
    for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
        if (metrics[METRIC_GPU_START].enabled[i] ||
            metrics[METRIC_GPU_DURATION].enabled[i])
        {
            cpugpuSync = true;
            break;
        }
    }
    // check if two passes are needed
    // GL_TIME_ELAPSED (gpu dur) and GL_SAMPLES_PASSED (pixels) cannot be nested
    if (!supportsTimestamp &&
        metrics[METRIC_GPU_DURATION].enabled[QUERY_BOUNDARY_FRAME] &&
       (metrics[METRIC_GPU_DURATION].enabled[QUERY_BOUNDARY_CALL] ||
        metrics[METRIC_GPU_DURATION].enabled[QUERY_BOUNDARY_DRAWCALL]))
    {
        twoPasses = true;
    }
    if (metrics[METRIC_GPU_PIXELS].enabled[QUERY_BOUNDARY_FRAME] &&
       (metrics[METRIC_GPU_PIXELS].enabled[QUERY_BOUNDARY_CALL] ||
        metrics[METRIC_GPU_PIXELS].enabled[QUERY_BOUNDARY_DRAWCALL]))
    {
        twoPasses = true;
    }

    curPass = 1;
    return twoPasses ? 2 : 1;
}

void MetricBackend_common::beginPass() {
    if (curPass == 1) {
        for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
            for (auto &m : metrics) {
                if (m.enabled[i]) m.profiled[i] = true;
            }
        }
        // profile frames in first pass
        if (twoPasses) {
            if (!supportsTimestamp) {
                metrics[METRIC_GPU_DURATION].profiled[QUERY_BOUNDARY_DRAWCALL] = false;
                metrics[METRIC_GPU_DURATION].profiled[QUERY_BOUNDARY_CALL] = false;
            }
            metrics[METRIC_GPU_PIXELS].profiled[QUERY_BOUNDARY_DRAWCALL] = false;
            metrics[METRIC_GPU_PIXELS].profiled[QUERY_BOUNDARY_CALL] = false;
        }
    }
    else if (curPass == 2) {
        for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
            for (auto &m : metrics) {
                m.profiled[i] = false;
            }
        }
        // profile calls/draw calls in second pass
        if (!supportsTimestamp) {
            if (metrics[METRIC_GPU_DURATION].enabled[QUERY_BOUNDARY_DRAWCALL]) {
                metrics[METRIC_GPU_DURATION].profiled[QUERY_BOUNDARY_DRAWCALL] = true;
            }
            if (metrics[METRIC_GPU_DURATION].enabled[QUERY_BOUNDARY_CALL]) {
                metrics[METRIC_GPU_DURATION].profiled[QUERY_BOUNDARY_CALL] = true;
            }
        }
        if (metrics[METRIC_GPU_PIXELS].enabled[QUERY_BOUNDARY_DRAWCALL]) {
            metrics[METRIC_GPU_PIXELS].profiled[QUERY_BOUNDARY_DRAWCALL] = true;
        }
        if (metrics[METRIC_GPU_PIXELS].enabled[QUERY_BOUNDARY_CALL]) {
            metrics[METRIC_GPU_PIXELS].profiled[QUERY_BOUNDARY_CALL] = true;
        }
    }
    // setup times
    cpuTimeScale = 1.0E9 / getTimeFrequency();
    baseTime = getCurrentTime() * cpuTimeScale;
}

void MetricBackend_common::processQueries() {
    int64_t gpuStart, gpuEnd, pixels;
    for (int i = 0; i < QUERY_BOUNDARY_LIST_END; i++) {
        QueryBoundary boundary = static_cast<QueryBoundary>(i);
        while (!queries[i].empty()) {
            auto &query = queries[i].front();
            if (metrics[METRIC_GPU_START].profiled[i]) {
                glGetQueryObjecti64v(query[QUERY_GPU_START], GL_QUERY_RESULT,
                                     &gpuStart);
                int64_t value = gpuStart - baseTime;
                data[METRIC_GPU_START][i]->addData(boundary, value);
            }
            if (metrics[METRIC_GPU_DURATION].profiled[i]) {
                if (supportsTimestamp) {
                    glGetQueryObjecti64v(query[QUERY_GPU_DURATION], GL_QUERY_RESULT,
                                         &gpuEnd);
                    gpuEnd -= gpuStart;
                } else {
                    glGetQueryObjecti64vEXT(query[QUERY_GPU_DURATION], GL_QUERY_RESULT,
                                            &gpuEnd);
                }
                data[METRIC_GPU_DURATION][i]->addData(boundary, gpuEnd);
            }
            if (metrics[METRIC_GPU_PIXELS].profiled[i]) {
                if (supportsTimestamp) {
                    glGetQueryObjecti64v(query[QUERY_OCCLUSION], GL_QUERY_RESULT, &pixels);
                } else if (supportsElapsed) {
                    glGetQueryObjecti64vEXT(query[QUERY_OCCLUSION], GL_QUERY_RESULT, &pixels);
                } else {
                    uint32_t pixels32;
                    glGetQueryObjectuiv(query[QUERY_OCCLUSION], GL_QUERY_RESULT, &pixels32);
                    pixels = static_cast<int64_t>(pixels32);
                }
                data[METRIC_GPU_PIXELS][i]->addData(boundary, pixels);
            }
            glDeleteQueries(QUERY_LIST_END, query.data());
            queries[i].pop();
        }
    }
}

void MetricBackend_common::endPass() {
    // process rest of the queries (it can be the last frame)
    // If context is destroyed explicitly in trace file
    // this is not going to work :(
    processQueries();
    curPass++;
}

void MetricBackend_common::beginQuery(QueryBoundary boundary) {
    // GPU related
    if (glQueriesNeeded[boundary]) {
        std::array<GLuint, QUERY_LIST_END> query;
        glGenQueries(QUERY_LIST_END, query.data());

        if (metrics[METRIC_GPU_START].profiled[boundary] ||
           (metrics[METRIC_GPU_DURATION].profiled[boundary] && supportsTimestamp))
        {
            glQueryCounter(query[QUERY_GPU_START], GL_TIMESTAMP);
        }
        if (metrics[METRIC_GPU_DURATION].profiled[boundary] && !supportsTimestamp) {
            glBeginQuery(GL_TIME_ELAPSED, query[QUERY_GPU_DURATION]);
        }
        if (metrics[METRIC_GPU_PIXELS].profiled[boundary]) {
            glBeginQuery(GL_SAMPLES_PASSED, query[QUERY_OCCLUSION]);
        }
        queries[boundary].push(std::move(query));
    }


    // CPU related
    if (metrics[METRIC_CPU_START].profiled[boundary] ||
        metrics[METRIC_CPU_DURATION].profiled[boundary])
    {
        cpuStart[boundary] = getCurrentTime();
        if (metrics[METRIC_CPU_START].profiled[boundary]) {
            int64_t time = cpuStart[boundary] * cpuTimeScale - baseTime;
            data[METRIC_CPU_START][boundary]->addData(boundary, time);
        }
    }
    if (metrics[METRIC_CPU_VSIZE_START].profiled[boundary] ||
        metrics[METRIC_CPU_VSIZE_DURATION].profiled[boundary])
    {
        vsizeStart[boundary] = os::getVsize();
        if (metrics[METRIC_CPU_VSIZE_START].profiled[boundary]) {
            int64_t time = vsizeStart[boundary];
            data[METRIC_CPU_VSIZE_START][boundary]->addData(boundary, time);
        }
    }
    if (metrics[METRIC_CPU_RSS_START].profiled[boundary] ||
        metrics[METRIC_CPU_RSS_DURATION].profiled[boundary])
    {
        rssStart[boundary] = os::getRss();
        if (metrics[METRIC_CPU_RSS_START].profiled[boundary]) {
            int64_t time = rssStart[boundary];
            data[METRIC_CPU_RSS_START][boundary]->addData(boundary, time);
        }
    }
    // DRAWCALL is a CALL
    if (boundary == QUERY_BOUNDARY_DRAWCALL) beginQuery(QUERY_BOUNDARY_CALL);
}

void MetricBackend_common::endQuery(QueryBoundary boundary) {
    // CPU related
    if (metrics[METRIC_CPU_DURATION].profiled[boundary])
    {
        cpuEnd[boundary] = getCurrentTime();
        int64_t time = (cpuEnd[boundary] - cpuStart[boundary]) * cpuTimeScale;
        data[METRIC_CPU_DURATION][boundary]->addData(boundary, time);
    }
    if (metrics[METRIC_CPU_VSIZE_DURATION].profiled[boundary])
    {
        vsizeEnd[boundary] = os::getVsize();
        int64_t time = vsizeEnd[boundary] - vsizeStart[boundary];
        data[METRIC_CPU_VSIZE_DURATION][boundary]->addData(boundary, time);
    }
    if (metrics[METRIC_CPU_RSS_DURATION].profiled[boundary])
    {
        rssEnd[boundary] = os::getRss();
        int64_t time = rssEnd[boundary] - rssStart[boundary];
        data[METRIC_CPU_RSS_DURATION][boundary]->addData(boundary, time);
    }
    // GPU related
    if (glQueriesNeeded[boundary]) {
        std::array<GLuint, QUERY_LIST_END> &query = queries[boundary].back();
        if (metrics[METRIC_GPU_DURATION].profiled[boundary] && supportsTimestamp) {
            // GL_TIME_ELAPSED cannot be used in nested queries
            // so prefer this if timestamps are supported
            glQueryCounter(query[QUERY_GPU_DURATION], GL_TIMESTAMP);
        }
        if (metrics[METRIC_GPU_PIXELS].profiled[boundary]) {
            glEndQuery(GL_SAMPLES_PASSED);
        }
    }
    // DRAWCALL is a CALL
    if (boundary == QUERY_BOUNDARY_DRAWCALL) endQuery(QUERY_BOUNDARY_CALL);
    // clear queries after each frame
    if (boundary == QUERY_BOUNDARY_FRAME && glQueriesNeededAnyBoundary) {
        processQueries();
    }
}

void MetricBackend_common::enumDataQueryId(unsigned id, enumDataCallback callback,
                                           QueryBoundary boundary, void* userData) {
    for (int i = 0; i < METRIC_LIST_END; i++) {
        Metric_common &metric = metrics[i];
        if (metric.enabled[boundary]) {
            callback(&metric, id, data[i][boundary]->getData(boundary, id), 0,
                     userData);
        }
    }
}

void MetricBackend_common::enumData(enumDataCallback callback,
                                    QueryBoundary boundary, void* userData) {
    // REMOVE
}

unsigned MetricBackend_common::getNumPasses() {
    return twoPasses ? 2 : 1;
}

unsigned MetricBackend_common::getLastQueryId() {
    return 0;
}

MetricBackend_common&
MetricBackend_common::getInstance(glretrace::Context* context) {
    static MetricBackend_common backend(context);
    return backend;
}
