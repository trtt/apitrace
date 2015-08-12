/**************************************************************************
 *
 * Copyright 2015 Alexander Trukhin
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#pragma once

#include <memory>
#include <vector>
#include <map>
#include <string>

#include "glproc.hpp"
#include "metric_backend.hpp"
#include "glretrace.hpp"

#define INTEL_NAME_LENGTH 256 // metric name with max 256 chars
#define INTEL_DESC_LENGTH 1024 // description max 1024 chars

class Metric_INTEL_perfquery : public Metric
{
private:
    unsigned group, id;
    unsigned offset;
    MetricNumType nType;
    bool precached;

    void precache();

public:
    Metric_INTEL_perfquery(unsigned g, unsigned i) : group(g), id(i),
                                                     offset(0),
                                                     nType(CNT_NUM_UINT),
                                                     precached(false) {}

    unsigned getOffset();


    unsigned getId();

    unsigned getGroupId();

    std::string getName();

    std::string getDescription();

    MetricNumType getNumType();

    MetricType getType();
};


class MetricBackend_INTEL_perfquery : public MetricBackend
{
private:
    class DataCollector
    {
        private:
            std::vector<std::vector<unsigned char*>> data;
            std::map<unsigned, unsigned> eventMap; // drawCallId <-> callId
            unsigned curPass;
            unsigned curEvent;

        public:
            DataCollector() : curPass(0), curEvent(0) {}

            ~DataCollector();

            unsigned char* newDataBuffer(unsigned event, size_t size);

            void endPass();

            unsigned char* getDataBuffer(unsigned pass, unsigned event);
    };

private:
    // map from query id to its Metric list
    std::map<unsigned, std::vector<Metric_INTEL_perfquery>> passes[2];
    /* curQueryMetrics -- iterator through passes */
    std::map<unsigned, std::vector<Metric_INTEL_perfquery>>::iterator curQueryMetrics;
    unsigned curQuery;
    bool supported;
    bool perFrame;
    int numPasses;
    int numFramePasses;
    int curPass;
    unsigned curEvent; // Currently evaluated event
    DataCollector collector;
    /* nameLookup for querying metrics by name */
    static std::map<std::string, std::pair<unsigned, unsigned>> nameLookup;

    MetricBackend_INTEL_perfquery(glretrace::Context* context);

    MetricBackend_INTEL_perfquery(MetricBackend_INTEL_perfquery const&) = delete;

    void operator=(MetricBackend_INTEL_perfquery const&)            = delete;

    void freeQuery(unsigned event); // collect metrics data from the query

    static void populateLookupGroups(unsigned group, int error, void* userData);

    static void populateLookupMetrics(Metric* metric, int error, void* userData);

public:
    bool isSupported();

    void enumGroups(enumGroupsCallback callback, void* userData = nullptr);

    void enumMetrics(unsigned group, enumMetricsCallback callback,
                     void* userData = nullptr);

    std::unique_ptr<Metric> getMetricById(unsigned groupId, unsigned metricId);

    std::unique_ptr<Metric> getMetricByName(std::string metricName);

    std::string getGroupName(unsigned group);

    int enableMetric(Metric* metric, QueryBoundary pollingRule = QUERY_BOUNDARY_DRAWCALL);

    unsigned generatePasses();

    void beginPass();

    void endPass();

    void beginQuery(QueryBoundary boundary = QUERY_BOUNDARY_DRAWCALL);

    void endQuery(QueryBoundary boundary = QUERY_BOUNDARY_DRAWCALL);

    void enumDataQueryId(unsigned id, enumDataCallback callback,
                         QueryBoundary boundary,
                         void* userData = nullptr);

    void enumData(enumDataCallback callback, QueryBoundary boundary,
                  void* userData = nullptr);

    unsigned getNumPasses();

    unsigned getLastQueryId();

    static MetricBackend_INTEL_perfquery& getInstance(glretrace::Context* context);
};

