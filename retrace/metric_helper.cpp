#include <string>
#include <vector>
#include <regex>
#include <iostream>

#include "retrace.hpp"
#include "metric_backend.hpp"
#include "metric_writer.hpp"
#include "metric_backend_amd_perfmon.hpp"

namespace glretrace {

bool metricBackendsSetup = false;
std::vector<MetricBackend*> metricBackends; // to be populated in initContext()
MetricBackend* curMetricBackend = nullptr; // backend active in the current pass
MetricWriter profiler(&metricBackends);

MetricBackend* getBackend(std::string backendName) {
    // to be populated with backends
    Context *currentContext = getCurrentContext();
    if (backendName == "GL_AMD_performance_monitor") return &MetricBackend_AMD_perfmon::getInstance(currentContext);
    else return nullptr;
}

bool
isLastPass() {
    return ((retrace::numPasses - 1) <= retrace::curPass);
}

void enableMetricsFromCLI() {
    const std::regex rOuter("([^:]+):\\s*([^;]*);?"); // backend: (...)
    const std::regex rInner("\\s*\\[\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\](\\*)?\\s*,?"
                            "|\\s*([^;\\*,]+)(\\*)?,?"); // [g, i]*? | metric*?
    bool perDraw;
    std::unique_ptr<Metric> p;
    std::string metricName;

    auto rOuter_it = std::cregex_token_iterator(retrace::profilingWithMetricsString,
        retrace::profilingWithMetricsString+std::strlen(retrace::profilingWithMetricsString),
        rOuter, {1,2});
    auto rOuter_end = std::cregex_token_iterator();
    while (rOuter_it != rOuter_end) {
        std::string backendName = (rOuter_it++)->str();
        MetricBackend* backend = getBackend(backendName);
        if (!backend) {
            std::cerr << "Warning: No backend \"" << backendName << "\"."
                      << std::endl;
            rOuter_it++;
            continue;
        }
        if (!backend->isSupported()) {
            std::cerr << "Warning: Backend \"" << backendName
                      << "\" is not supported." << std::endl;
            rOuter_it++;
            continue;
        }
        metricBackends.push_back(backend);

        auto rInner_it = std::cregex_token_iterator(rOuter_it->first, rOuter_it->second,
                                                    rInner, {1,2,3,4,5});
        auto rInner_end = std::cregex_token_iterator();
        while (rInner_it != rInner_end) {
            if (rInner_it->matched) {
                std::string groupId = (rInner_it++)->str();
                std::string metricId = (rInner_it++)->str();
                perDraw = ((rInner_it++)->matched) ? false : true;
                rInner_it++; // +/+= operators not supported
                rInner_it++;
                p = backend->getMetricById(std::stoi(groupId), std::stoi(metricId));
                metricName = "[" + groupId + ", " + metricId + "]";
            } else {
                rInner_it++;
                rInner_it++;
                rInner_it++;
                metricName = (rInner_it++)->str();
                p = backend->getMetricByName(metricName);
                perDraw = ((rInner_it++)->matched) ? false : true;
            }

            if (!p) {
                std::cerr << "Warning: No metric \"" << metricName
                    << "\"." << std::endl;
                continue;
            }
            int error = backend->enableMetric(p.get(), perDraw);
            if (error) {
                std::cerr << "Warning: Metric " << metricName << " not enabled"
                             " (error " << error << ")." << std::endl;
            }
        }
        rOuter_it++;
    }
}

} /* namespace glretrace */
