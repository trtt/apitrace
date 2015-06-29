#include <string>
#include <vector>

#include "metric_backend.hpp"

namespace glretrace {

bool metricBackendsSetup = false;
std::vector<MetricBackend*> metricBackends; // to be populated in initContext()
MetricBackend* curMetricBackend = nullptr; // backend active in the current pass

MetricBackend* getBackend(std::string backendName) {
    return nullptr; // to be populated with backends
}

} /* namespace glretrace */
