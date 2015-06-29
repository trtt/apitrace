#include <string>
#include <vector>

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

} /* namespace glretrace */
