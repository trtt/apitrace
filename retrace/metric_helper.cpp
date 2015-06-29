#include <string>
#include <vector>
#include <regex>
#include <iostream>

#include "retrace.hpp"
#include "metric_backend.hpp"
#include "metric_writer.hpp"

namespace glretrace {

bool metricBackendsSetup = false;
std::vector<MetricBackend*> metricBackends; // to be populated in initContext()
MetricBackend* curMetricBackend = nullptr; // backend active in the current pass
MetricWriter profiler(&metricBackends);

MetricBackend* getBackend(std::string backendName) {
    return nullptr; // to be populated with backends
}

bool
isLastPass() {
    return ((retrace::numPasses - 1) <= retrace::curPass);
}

void enableMetricsFromCLI() {
    const std::regex rOuter("\"([^\"]*)\"\\s*:\\s*\\[([^\"]*)");
    const std::regex rInner("\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(0|1)\\s*\\],?");
    auto rOuter_it = std::cregex_token_iterator(retrace::profilingWithMetricsString, retrace::profilingWithMetricsString+std::strlen(retrace::profilingWithMetricsString), rOuter, {1,2});
    auto rOuter_end = std::cregex_token_iterator();
    while (rOuter_it != rOuter_end) {
        std::string backendName = (rOuter_it++)->str();
        MetricBackend* backend = getBackend(backendName);
        if (!backend) {
            std::cerr << "Warning: No backend \"" << backendName << "\"." << std::endl;
            rOuter_it++;
            continue;
        }
        if (!backend->isSupported()) {
            std::cerr << "Warning: Backend \"" << backendName << "\" is not supported." << std::endl;
            rOuter_it++;
            continue;
        }
        metricBackends.push_back(backend);
        auto rInner_it = std::cregex_token_iterator(rOuter_it->first, rOuter_it->second, rInner, {1,2,3});
        auto rInner_end = std::cregex_token_iterator();
        while (rInner_it != rInner_end) {
            unsigned groupId = std::stoi((rInner_it++)->str());
            unsigned metricId = std::stoi((rInner_it++)->str());
            bool perDraw = std::stoi((rInner_it++)->str());
            Metric metric = Metric(groupId, metricId);
            int error = backend->enableMetric(&metric, perDraw);
            if (error) {
                std::cerr << "Warning: Metric [" << groupId << ", " << metricId << "] not enabled: error " << error << std::endl;
            }
        }
        rOuter_it++;
    }
}

} /* namespace glretrace */
