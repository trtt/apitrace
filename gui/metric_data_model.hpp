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

class QueryItem
{
public:
    void addMetric(QVariant &&data);
    QVariant getMetric(int index) const;
private:
    // QVariant is probably not the best thing to use here
    // TODO: external storage for metrics
    QList<QVariant> m_metrics;
};

// currently only draw calls
class CallItem : public QueryItem
{
public:
    CallItem(unsigned no, unsigned program, unsigned frame, QString name);
    unsigned no() const;
    unsigned program() const;
    unsigned frame() const;
    QString name() const;

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
        std::string getString(T id) {
            return strings[static_cast<typename decltype(stringLookupTable)::size_type>(id)];
        }
    };
    static StringTable<int16_t> nameTable;

    unsigned m_no;
    unsigned m_program;
    unsigned m_frame;
    int16_t m_nameTableEntry;
};


class FrameItem : public QueryItem
{
public:
    FrameItem(unsigned no) : m_no(no) {};
    unsigned no() const;

private:
    unsigned m_no;
};


class MetricFrameDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MetricFrameDataModel(QObject *parent = 0) : init(false) {}

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void addMetricsData(QTextStream &stream, MetricOutputLookup &lookup);
private:
    enum PreColumns {
        COLUMN_ID = 0,
        COLUMN_METRICS_BEGIN
    };

    QList<MetricItem*> m_metrics;
    std::vector<FrameItem> m_data;
    bool init;
};



class MetricCallDataModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MetricCallDataModel(QObject *parent = 0) : init(false) {}

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void addMetricsData(QTextStream &stream, MetricOutputLookup &lookup);
private:
    enum PreColumns {
        COLUMN_ID = 0,
        COLUMN_PROGRAM,
        COLUMN_FRAME,
        COLUMN_NAME,
        COLUMN_METRICS_BEGIN
    };

    QList<MetricItem*> m_metrics;
    std::vector<CallItem> m_data;
    bool init;
};
