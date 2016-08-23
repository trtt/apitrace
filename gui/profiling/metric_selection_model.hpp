/**************************************************************************
 *
 * Copyright 2016 Alexander Trukhin
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

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>
#include <QSet>
#include <QVariant>
#include <QIODevice>

enum MetricNumType {
    CNT_NUM_UINT = 0,
    CNT_NUM_FLOAT,
    CNT_NUM_UINT64,
    CNT_NUM_DOUBLE,
    CNT_NUM_BOOL,
    CNT_NUM_INT64
};

enum MetricType {
    CNT_TYPE_GENERIC = 0,
    CNT_TYPE_NUM,
    CNT_TYPE_DURATION,
    CNT_TYPE_PERCENT,
    CNT_TYPE_TIMESTAMP,
    CNT_TYPE_OTHER
};

class GroupItem;
class MetricItem;

typedef QHash<QString, QHash<QString, MetricItem*>> MetricOutputLookup;

class BackendItem
{
public:
    explicit BackendItem(const QString name) : m_name(name) {};
    ~BackendItem();

    void appendChild(GroupItem *child);

    GroupItem* child(int row);
    int childCount() const;
    QString getName() const;

    int row;

private:
    QList<GroupItem*> m_childItems;
    QString m_name;
};

class GroupItem
{
public:
    explicit GroupItem(const int id, const QString name,
                       BackendItem* parentItem)
        : m_id(id), m_name(name), m_parentItem(parentItem) {};
    ~GroupItem();

    void appendChild(MetricItem* child);
    MetricItem* child(int row);
    int childCount() const;
    int getId() const;
    QString getName() const;
    BackendItem* parentItem();

    int row;

private:
    QList<MetricItem*> m_childItems;
    int m_id;
    QString m_name;
    BackendItem* m_parentItem;
};

class MetricItem
{
public:
    explicit MetricItem(const int id, const QString name,
                        const QString desc, const MetricNumType nType,
                        GroupItem* parentItem)
        : m_id(id), m_name(name), m_desc(desc), m_nType(nType), 
          m_parentItem(parentItem) {};
    ~MetricItem() = default;

    int getId() const;
    QString getName() const;
    QString getDesc() const;
    MetricNumType getNumType() const;
    GroupItem* parentItem();

private:
    int m_id;
    QString m_name, m_desc;
    MetricNumType m_nType;
    GroupItem* m_parentItem;
};

class MetricSelectionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit MetricSelectionModel(QIODevice &io, QObject *parent = 0);
    ~MetricSelectionModel();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex & index,
                                     const QVariant & value,
                                     int role = Qt::EditRole) Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QModelIndex parentColumn(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void generateMetricList(QString& cliOptionFrame,
                            QString& cliOptionCall);
    const MetricOutputLookup& selectedForCalls() const;
    const MetricOutputLookup& selectedForFrames() const;

    void enableCalls();
    void disableCalls();
    void enableFrames();
    void disableFrames();

    bool isProfilingCalls() const { return profilingCalls; }
    bool isProfilingFrames() const { return profilingFrames; }
    bool callsProfiled() const { return m_callsProfiled; }
    bool framesProfiled() const { return m_framesProfiled; }

private:
    enum MetricSelectionItem {
        MSBackend = 0,
        MSGroup,
        MSMetric
    };

    void checkItem(const QModelIndex & index, Qt::CheckState state);
    void checkUpdateParent(const QModelIndex & index, Qt::CheckState state);
    bool isPartiallyChecked(const QModelIndex & index) const;
    void parseData(QIODevice &io);

    QList<BackendItem*> backends;
    QHash<void*, MetricSelectionItem> itemType;
    QSet<QModelIndex> selected;
    QSet<QModelIndex> needed; // for CPU/GPU times
    QSet<QModelIndex> profiled;
    QHash<QModelIndex, int> childNodesSelected;
    MetricOutputLookup toBeProfiledCalls;
    MetricOutputLookup toBeProfiledFrames;
    bool profilingCalls = false;
    bool profilingFrames = false;
    bool m_callsProfiled = false;
    bool m_framesProfiled = false;
};
