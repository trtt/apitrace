#include <vector>
#include <string>

enum CounterNumType {
    CNT_UINT = 0,
    CNT_FLOAT,
    CNT_UINT64,
    CNT_DOUBLE,
};

typedef void (*enumGroupsCallback)(unsigned group);

class Counter
{
    private:
        unsigned group, id;
    public:
        Counter(unsigned g, unsigned i) : group(g), id(i) {}
        unsigned getId() {
            return id;
        }
        unsigned getGroupId() {
            return group;
        }
        std::string getName() {
            int length;
            std::string name;
            glGetPerfMonitorCounterStringAMD(group, id, 0, &length, nullptr);
            name.resize(length);
            glGetPerfMonitorCounterStringAMD(group, id, length, 0, &name[0]);
            return name;
        }

        GLenum getSize() {
            GLenum type;
            glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
            if (type == GL_UNSIGNED_INT) return sizeof(GLuint);
            else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return sizeof(GLfloat);
            else if (type == GL_UNSIGNED_INT64_AMD) return sizeof(uint64_t);
            return sizeof(GLuint);
        }

        CounterNumType getNumType() {
            GLenum type;
            glGetPerfMonitorCounterInfoAMD(group, id, GL_COUNTER_TYPE_AMD, &type);
            if (type == GL_UNSIGNED_INT) return CNT_UINT;
            else if (type == GL_FLOAT || type == GL_PERCENTAGE_AMD) return CNT_FLOAT;
            else if (type == GL_UNSIGNED_INT64_AMD) return CNT_UINT64;
            return CNT_UINT;
        }

};


class DataCollector
{
private:
    std::vector<std::vector<unsigned*>> data;
    int curPass = 0;
    int curEvent = 0;
public:
    ~DataCollector() {
        for (std::vector<unsigned*> t1 : data) {
            for (unsigned* t2 : t1) {
                delete[] t2;
            }
        }
    }

    unsigned* newDataBuffer(size_t size) {
        if (curEvent == 0) {
            std::vector<unsigned*> vec(1, new unsigned[size]);
            data.push_back(vec);
        } else {
            data[curPass].push_back(new unsigned[size]);
        }

        return data[curPass][curEvent++];
    }

    void endPass() {
        curPass++;
        curEvent = 0;
    }

    unsigned* getDataBuffer(int pass, int event) {
        return data[pass][event];
    }

    int getNumEvents() {
        return data[0].size();
    }

    int getLastEvent() {
        return (curEvent - 1);
    }
};

typedef void (*enumCountersCallback)(Counter* counter);
typedef void (*enumDataCallback)(Counter* counter, int event, void* data);

class Api_GL_AMD_performance_monitor
{
private:
    unsigned monitors[2]; // For cycling, using 2 in current implementation
    int curMonitor;
    bool firstRound;
    std::vector<std::vector<Counter>> passes;
    int numPasses = 0;
    int curPass = 0;
    DataCollector collector;

public:
    std::vector<Counter> metrics;

    Api_GL_AMD_performance_monitor() {

    }

    void enumGroups(enumGroupsCallback callback) {
        std::vector<unsigned> groups;
        int num_groups;
        glGetPerfMonitorGroupsAMD(&num_groups, 0, nullptr);
        groups.resize(num_groups);
        glGetPerfMonitorGroupsAMD(nullptr, num_groups, &groups[0]);
        for(unsigned g : groups) {
            callback(g);
        }
    }

    void enumCounters(unsigned group, enumCountersCallback callback) {
        std::vector<unsigned> counters;
        int num_counters;
        Counter counter(0,0);
        glGetPerfMonitorCountersAMD(group, &num_counters, nullptr,  0, nullptr);
        counters.resize(num_counters);
        glGetPerfMonitorCountersAMD(group, nullptr, nullptr, num_counters, &counters[0]);
        for(unsigned c : counters) {
            counter = Counter(group, c);
            callback(&counter);
        }
    }

    void enableCounter(Counter* counter) {
        metrics.push_back(*counter);
    }

    bool testCounters(std::vector<Counter>* counters) {
        unsigned monitor;
        unsigned id;
        glGenPerfMonitorsAMD(1, &monitor);
        for (Counter c : *counters) {
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

    unsigned generatePasses() {
        std::vector<Counter> copyMetrics(metrics);
        std::vector<Counter> newPass;
        while (!copyMetrics.empty()) {
            std::vector<Counter>::iterator it = copyMetrics.begin();
            while (it != copyMetrics.end()) {
                newPass.push_back(*it);
                if (!testCounters(&newPass)) break;
                it = copyMetrics.erase(it);
            }
            passes.push_back(newPass);
            newPass.clear();
        }
        numPasses = passes.size();
        return numPasses;
    }

    void beginPass() {
        if (curPass == 0) {
            generatePasses();
        }
        glGenPerfMonitorsAMD(2, monitors);
        unsigned id;
        for (Counter c : passes[curPass]) {
            id = c.getId();
            glSelectPerfMonitorCountersAMD(monitors[0], 1, c.getGroupId(), 1, &id);
            glSelectPerfMonitorCountersAMD(monitors[1], 1, c.getGroupId(), 1, &id);
        }
        curMonitor = 0;
        firstRound = 1;
    }

    void endPass() {
        freeMonitor(monitors[0]);
        freeMonitor(monitors[1]);
        glDeletePerfMonitorsAMD(2, monitors);
        curPass++;
        collector.endPass();
        //fprintf(stderr, "Events: %i\n", collector.getNumEvents());
    }

    void beginQuery() {
        if (!firstRound) freeMonitor(monitors[curMonitor]);
        glBeginPerfMonitorAMD(monitors[curMonitor]);
    }

    void endQuery() {
        glEndPerfMonitorAMD(monitors[curMonitor]);
        curMonitor++;
        if (curMonitor > 1) firstRound = 0;
        curMonitor &= 1;
    }

    void freeMonitor(unsigned monitor) {
        glFlush();
        GLuint dataAvail = 0;
        while (!dataAvail) {
            glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AVAILABLE_AMD, sizeof(GLuint), &dataAvail, nullptr);
        }
        GLuint size;
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_SIZE_AMD, sizeof(GLuint), &size, nullptr);
        // collect data
        glGetPerfMonitorCounterDataAMD(monitor, GL_PERFMON_RESULT_AMD, size, collector.newDataBuffer(size), nullptr);
    }

    void enumDataQueryId(int id, enumDataCallback callback) {
        for (int j = 0; j < numPasses; j++) {
            unsigned* buf = collector.getDataBuffer(j, id);
            unsigned offset = 0;
            for (Counter c : passes[j]) {
                callback(&c, id, &buf[offset]);
                offset += c.getSize();
            }
        }
    }
    void enumData(enumDataCallback callback) {
        int numEvents = collector.getNumEvents();
        for (int i = 0; i < numEvents; i++) {
            enumDataQueryId(i, callback);
        }
    }

    int getNumPasses() {
        return numPasses;
    }

    bool lastPass() {
        return (numPasses-1 == curPass);
    }

    int getLastQueryId() {
        return collector.getLastEvent();
    }
};
