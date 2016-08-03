#pragma once

#include <queue>
#include <string>
#include <unordered_map>

#include "metric_selection_model.hpp"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>
#include <QSet>
#include <QVariant>
#include <QTextStream>

enum QueryBoundary {
    QUERY_BOUNDARY_DRAWCALL = 0,
    QUERY_BOUNDARY_FRAME,
    QUERY_BOUNDARY_CALL,
    QUERY_BOUNDARY_LIST_END
};

class MetricStorage
{
public:
    /*
    * making storage for metrics explicitly (via new)
    * so std::vector does not move around
    */
    MetricStorage(MetricItem* item) : item(item) {
        data = new std::vector<float>();
    }
    ~MetricStorage() { if (data) delete data; }
    MetricStorage(MetricStorage&& ms) : data(ms.data), item(ms.item) { ms.data = nullptr; }

    void addMetricData(float data);
    float getMetricData(unsigned index) const { return (*data)[index]; }
    MetricItem* metric() const { return item; }
    const std::vector<float>* vector() const { return data; }

private:
    std::vector<float>* data;
    MetricItem* item;
};


class DrawcallStorage
{
public:
    enum TimestampType {
        TimestampCPU,
        TimestampGPU,
        TimestampSize
    };

    DrawcallStorage() {}

    std::string name(unsigned index) const { return nameTable.getString(nameHash[index]); }
    unsigned no(unsigned index) const { return s_no[index]; }
    unsigned program(unsigned index) const { return s_program[index]; }
    unsigned frame(unsigned index) const { return s_frame[index]; }

    unsigned size() const { return s_no.size(); }

    void addDrawcall(unsigned no, unsigned program, unsigned frame, QString name);

    void addTimestamp(qlonglong time, TimestampType type);

    std::vector<unsigned>* timestampHData(TimestampType t) { return &s_timestampH[t]; }
    std::vector<unsigned>* timestampLData(TimestampType t) { return &s_timestampL[t]; }
    std::vector<unsigned>* programData() { return &s_program; }
    std::vector<unsigned>* nameHashData() { return &nameHash; }
    unsigned nameHashNumEntries() const { return nameTable.size(); }

private:
    template<typename T>
    class StringTable
    {
    private:
        std::deque<std::string> strings;
        std::unordered_map<std::string, T> stringLookupTable;

    public:
        T getId(const std::string &str) {
            auto res = stringLookupTable.find(str);
            T index;
            if (res == stringLookupTable.end()) {
                index = static_cast<T>(strings.size());
                strings.push_back(str);
                stringLookupTable[str] = index;
            } else {
                index = res->second;
            }
            return index;
        }
        std::string getString(T id) const {
            return strings[static_cast<typename decltype(stringLookupTable)::size_type>(id)];
        }
        T size() const {
            return static_cast<T>(strings.size());
        }
    };
    StringTable<unsigned> nameTable;

    std::vector<unsigned> nameHash;
    std::vector<unsigned> s_no;
    std::vector<unsigned> s_program;
    std::vector<unsigned> s_frame;

    // 64 bit timestamp's 32 bit High and Low for shaders
    std::vector<unsigned> s_timestampH[TimestampSize];
    std::vector<unsigned> s_timestampL[TimestampSize];
};

class MetricCallDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MetricCallDataModel(QObject *parent = 0)
        : init(false), durationCPUIndexInMetrics(0), durationGPUIndexInMetrics(0) {}

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void addMetricsData(QTextStream &stream, MetricOutputLookup &lookup);

    const std::vector<float>* durationDataCPU() const;
    const std::vector<float>* durationDataGPU() const;

    DrawcallStorage& calls() { return m_calls; }
    std::vector<MetricStorage>& metrics() { return m_metrics; }

private:
    enum PreColumns {
        COLUMN_ID = 0,
        COLUMN_PROGRAM,
        COLUMN_FRAME,
        COLUMN_NAME,
        COLUMN_METRICS_BEGIN
    };

    std::vector<MetricStorage> m_metrics;
    DrawcallStorage m_calls;
    bool init;
    unsigned durationCPUIndexInMetrics;
    unsigned durationGPUIndexInMetrics;
};
