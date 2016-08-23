#include "metric_selection_model.hpp"

#include <iostream>
#include <QPair>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QRegularExpression>

BackendItem::~BackendItem() {
    for (auto &g : m_childItems) {
        delete g;
    }
}

void BackendItem::appendChild(GroupItem* child) {
    m_childItems.push_back(child);
    child->row = m_childItems.size() - 1;
}

GroupItem* BackendItem::child(int row) {
    return m_childItems[row];
}

int BackendItem::childCount() const {
    return m_childItems.size();
}

QString BackendItem::getName() const {
    return m_name;
}

GroupItem::~GroupItem() {
    for (auto &g : m_childItems) {
        delete g;
    }
}

void GroupItem::appendChild(MetricItem* child) {
    m_childItems.push_back(child);
}

MetricItem* GroupItem::child(int row) {
    return m_childItems[row];
}

int GroupItem::childCount() const {
    return m_childItems.size();
}

int GroupItem::getId() const {
    return m_id;
}

QString GroupItem::getName() const {
    return m_name;
}

BackendItem* GroupItem::parentItem() {
    return m_parentItem;
}

int MetricItem::getId() const {
    return m_id;
}

QString MetricItem::getName() const {
    return m_name;
}

QString MetricItem::getDesc() const {
    return m_desc;
}

MetricNumType MetricItem::getNumType() const {
    return m_nType;
}

GroupItem* MetricItem::parentItem() {
    return m_parentItem;
}


MetricSelectionModel::MetricSelectionModel(QIODevice &io, QObject *parent)
    : QAbstractItemModel(parent)
{
    parseData(io);
}

MetricSelectionModel::~MetricSelectionModel() {
    for (auto &b : backends) {
        delete b;
    }
}

void MetricSelectionModel::parseData(QIODevice &io) {
    QTextStream stream(&io);
    stream.readLine();
    stream.readLine(); // read header
    while (!stream.atEnd()) {
        QRegularExpression reBackend("Backend (.*):$");
        QString line = stream.readLine();
        QRegularExpressionMatch match = reBackend.match(line);
        QString backendName = match.captured(1);
        // add backend
        BackendItem* backend = new BackendItem(backendName);
        backends.push_back(backend);
        backend->row = backends.size() - 1;
        itemType[backend] = MSBackend;

        QRegularExpression reGroup("Group #(\\d+): (.*)\\.$");
        stream.readLine(); // blank
        line = stream.readLine();
        match = reGroup.match(line);
        while (match.hasMatch()) {
            int groupId = match.captured(1).toInt();
            QString groupName = match.captured(2);
            // add group
            GroupItem* group = new GroupItem(groupId, groupName, backend);
            backend->appendChild(group);
            itemType[group] = MSGroup;

            QRegularExpression reMetric("Metric #(\\d+): (.*) \\(type: ([^,]*), num\\. type: ([^\\)]*)");
            QRegularExpression reDescription("Description: (.*)$");
            line = stream.readLine();
            match = reMetric.match(line);
            while (match.hasMatch()) {
                int metricId = match.captured(1).toInt();
                QString metricName = match.captured(2);
                QString metricNType = match.captured(4);
                MetricNumType mNType;
                if (metricNType == "CNT_NUM_FLOAT")        mNType = CNT_NUM_FLOAT;
                else if (metricNType == "CNT_NUM_UINT64")  mNType = CNT_NUM_UINT64;
                else if (metricNType == "CNT_NUM_DOUBLE")  mNType = CNT_NUM_DOUBLE;
                else if (metricNType == "CNT_NUM_BOOL")    mNType = CNT_NUM_BOOL;
                else if (metricNType == "CNT_NUM_INT64")   mNType = CNT_NUM_INT64;
                else mNType = CNT_NUM_UINT;
                line = stream.readLine();
                match = reDescription.match(line);
                QString metricDesc = match.captured(1);
                // add metric
                MetricItem* metric = new MetricItem(metricId, metricName,
                                                  metricDesc, mNType, group);
                group->appendChild(metric);
                itemType[metric] = MSMetric;

                // insert into needed metrics if necessary
                if ((metric->getName() == QLatin1String("CPU Start")) ||
                    (metric->getName() == QLatin1String("CPU Duration")) ||
                    (metric->getName() == QLatin1String("GPU Start")) ||
                    (metric->getName() == QLatin1String("GPU Duration")))
                {
                    auto index = createIndex(group->childCount() - 1, 2, metric);
                    needed.insert(index);
                    index = createIndex(group->childCount() - 1, 1, metric);
                    needed.insert(index);
                }

                line = stream.readLine();
                match = reMetric.match(line);
            }
            line = stream.readLine(); // blank line
            match = reGroup.match(line);
        }
    }

}

void MetricSelectionModel::enableCalls() {
    foreach (auto& i, needed) {
        if (i.column() == 2)
            checkItem(i, Qt::Checked);
    }
    profilingCalls = true;
    layoutChanged();
}

void MetricSelectionModel::disableCalls() {
    foreach (auto& i, selected) {
        if (i.column() == 2)
            checkItem(i, Qt::Unchecked);
    }
    profilingCalls = false;
    layoutChanged();
}

void MetricSelectionModel::enableFrames() {
    foreach (auto& i, needed) {
        if (i.column() == 1)
            checkItem(i, Qt::Checked);
    }
    profilingFrames = true;
    layoutChanged();
}

void MetricSelectionModel::disableFrames() {
    foreach (auto& i, selected) {
        if (i.column() == 1)
            checkItem(i, Qt::Unchecked);
    }
    profilingFrames = false;
    layoutChanged();
}

int MetricSelectionModel::columnCount(const QModelIndex &parent) const {
    return 4;
}


QModelIndex MetricSelectionModel::index(int row, int column,
                                        const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid()) {
        return createIndex(row, column, backends[row]);
    }

    void* item;
    switch(itemType[parent.internalPointer()]) {
        case MSBackend:
            item = (static_cast<BackendItem*>(parent.internalPointer())->child(row));
            return createIndex(row, column, item);
            break;
        case MSGroup:
            item = (static_cast<GroupItem*>(parent.internalPointer())->child(row));
            return createIndex(row, column, item);
            break;
        default:
            return QModelIndex();
            break;
    }
}

void MetricSelectionModel::checkUpdateParent(const QModelIndex & index, Qt::CheckState state) {
    QModelIndex parent = parentColumn(index);
    if (parent.isValid()) {
        QModelIndex pparent = parentColumn(parent);
        if (childNodesSelected[parent] == rowCount(parent)-1 && state == Qt::Unchecked) {
            childNodesSelected[pparent]--;
        }
        if (childNodesSelected[parent] == rowCount(parent) && state == Qt::Checked) {
            childNodesSelected[pparent]++;
        }
        emit dataChanged(pparent, pparent, {Qt::CheckStateRole});
    }
}

void MetricSelectionModel::checkItem(const QModelIndex & index, Qt::CheckState state) {
    int numChild = rowCount(index);
    if (!numChild && !profiled.contains(index)) {
        bool changed = true;
        QModelIndex parent = parentColumn(index);
        if (!selected.contains(index) && state == Qt::Checked)
        {
            selected.insert(index);
            childNodesSelected[parent]++;
        }
        else if (selected.contains(index) && state == Qt::Unchecked)
        {
            selected.remove(index);
            childNodesSelected[parent]--;
        }
        else changed = false;
        if (changed) {
            emit dataChanged(index, index, {Qt::CheckStateRole});
            emit dataChanged(parent, parent, {Qt::CheckStateRole});
            checkUpdateParent(index, state);
        }
    } else {
        for (int i = 0; i < numChild; i++) {
            checkItem(this->index(i, index.column(), index), state);
        }
    }
}

bool MetricSelectionModel::setData(const QModelIndex & index,
             const QVariant & value,
             int role)
{
    if (role == Qt::CheckStateRole) {
        checkItem(index, static_cast<Qt::CheckState>(value.toInt()));
        return true;
    }
    return false;
}

bool MetricSelectionModel::isPartiallyChecked(const QModelIndex &index) const {
    if (childNodesSelected[index]) return true;
    int numChild = rowCount(index);
    for (int i = 0; i < numChild; i++) {
        if (isPartiallyChecked(this->index(i, index.column(), index))) {
            return true;
        }
    }
    return false;
}

QVariant MetricSelectionModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (index.column() == 0 && role == Qt::DisplayRole) {
        void* intp = index.internalPointer();
        switch(itemType[intp]) {
            case MSBackend:
                return (static_cast<BackendItem*>(intp))->getName();
                break;
            case MSGroup:
                return (static_cast<GroupItem*>(intp))->getName();
                break;
            case MSMetric:
                return (static_cast<MetricItem*>(intp))->getName();
                break;
            default:
                return QVariant();
        }
    }
    else if ((index.column() == 1 || index.column() == 2)
              && role == Qt::CheckStateRole)
    {
        int numChild = rowCount(index);
        if (!numChild) {
            return selected.contains(index) || profiled.contains(index) ? Qt::Checked : Qt::Unchecked;
        }
        else {
            if (childNodesSelected[index] == numChild) return Qt::Checked;
            else if (isPartiallyChecked(index)) return Qt::PartiallyChecked;
            else return Qt::Unchecked;
        }
    }
    else if (index.column() == 3 && role == Qt::DisplayRole) {
        void* intp = index.internalPointer();
        switch(itemType[intp]) {
            case MSBackend:
                return "";
                break;
            case MSGroup:
                return "";
                break;
            case MSMetric:
                return (static_cast<MetricItem*>(intp))->getDesc();
                break;
            default:
                return QVariant();
        }
    }
    else return QVariant();
}

QVariant MetricSelectionModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case 0:
            return "Name";
        case 1:
            return "frames";
        case 2:
            return "draw calls";
        case 3:
            return "Description";
        default:
            return "";
        }
    }
    else return QVariant();
}

Qt::ItemFlags MetricSelectionModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable | Qt::ItemIsTristate;
    if (profiled.contains(index)) flags &= ~Qt::ItemIsEnabled;
    if (needed.contains(index)) flags &= ~Qt::ItemIsEnabled;
    if (!profilingCalls && index.column() == 2) flags &= ~Qt::ItemIsEnabled;
    if (!profilingFrames && index.column() == 1) flags &= ~Qt::ItemIsEnabled;
    return flags;
}

int MetricSelectionModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return backends.size();
    }
    void* intp = parent.internalPointer();
    switch (itemType[intp]) {
        case MSBackend:
            return static_cast<BackendItem*>(intp)->childCount();
        case MSGroup:
            return static_cast<GroupItem*>(intp)->childCount();
        default:
            return 0;
    }
}

QModelIndex MetricSelectionModel::parent(const QModelIndex &index) const {
    void* intp = index.internalPointer();
    switch(itemType[intp]) {
        case MSMetric:
        {
            MetricItem* metricItem = static_cast<MetricItem*>(intp);
            return createIndex(metricItem->parentItem()->row, 0, metricItem->parentItem());
        }
        case MSGroup:
        {
            GroupItem* groupItem = static_cast<GroupItem*>(intp);
            return createIndex(groupItem->parentItem()->row, 0, groupItem->parentItem());
        }
        default:
            return QModelIndex();
    }
    return QModelIndex();
}

QModelIndex MetricSelectionModel::parentColumn(const QModelIndex &index) const {
    QModelIndex parent = index.parent();
    return this->index(parent.row(), index.column(), parent.parent());
}

inline QString
stringFromHashGenerateLookup(const QHash<QString, QList<MetricItem*>>& hash,
                             MetricOutputLookup& lookup)
{
    QString result;
    for (auto it = hash.cbegin(); it != hash.cend(); ++it) {
        result += it.key() + ": ";
        for (auto &p : it.value()) {
            unsigned gId = p->parentItem()->getId();
            unsigned mId = p->getId();
            lookup[it.key()][p->getName()] = p;
            result += QString("[%1,%2],").arg(gId).arg(mId);
        }
        result += "; ";
    }
    return result;
}

void MetricSelectionModel::generateMetricList(QString& cliOptionFrame,
                            QString& cliOptionCall)
{
    QHash<QString, QList<MetricItem*>> mFramePts;
    QHash<QString, QList<MetricItem*>> mCallPts;
    for (auto &p : selected) {
        QString backendName = static_cast<BackendItem*>(p.parent().parent().internalPointer())->getName();
        if (p.column() == 1) {
            mFramePts[backendName].append(static_cast<MetricItem*>(p.internalPointer()));
        } else {
            mCallPts[backendName].append(static_cast<MetricItem*>(p.internalPointer()));
        }
        profiled.insert(p);
    }
    selected.clear();
    toBeProfiledCalls.clear();
    toBeProfiledFrames.clear();
    cliOptionFrame = "--pframes=" + stringFromHashGenerateLookup(mFramePts, toBeProfiledFrames);
    if (!toBeProfiledFrames.empty()) m_framesProfiled = true;
    cliOptionCall = "--pdrawcalls=" + stringFromHashGenerateLookup(mCallPts, toBeProfiledCalls);
    if (!toBeProfiledCalls.empty()) m_callsProfiled = true;
}

const MetricOutputLookup& MetricSelectionModel::selectedForCalls() const {
    return toBeProfiledCalls;
}

const MetricOutputLookup& MetricSelectionModel::selectedForFrames() const {
    return toBeProfiledFrames;
}

#include "metric_selection_model.moc"
