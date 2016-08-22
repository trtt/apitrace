#include "groupproxymodel.h"

inline uint qHash(const QVariant& key) {
    switch (key.type())
    {
        case QVariant::Int:
            return qHash(key.toInt());
            break;
        case QVariant::UInt:
            return qHash(key.toUInt());
            break;
        case QVariant::String:
            return qHash(key.toString());
            break;
        default:
            return 0;
    }
}


GroupProxyModel::GroupProxyModel(QObject *parent)
        : QAbstractProxyModel(parent), m_groupBy(GROUP_BY_INVALID)
{
}

void GroupProxyModel::setSourceModel(QAbstractItemModel *sourceModel) {
    QAbstractProxyModel::setSourceModel(sourceModel);
    setGroupBy(GROUP_BY_FRAME);
}

QModelIndex GroupProxyModel::mapFromSource(const QModelIndex &sourceIndex) const {
    return QModelIndex();
}

QModelIndex GroupProxyModel::mapToSource(const QModelIndex &proxyIndex) const {
    if (!sourceModel() || !proxyIndex.isValid())
        return QModelIndex();

    int sourceColumn = proxyIndex.column();
    if (proxyIndex.column() == 0) {
        sourceColumn = m_groupBy;
    } else if (proxyIndex.column() < m_groupBy + 1) {
        sourceColumn--;
    }

    int sourceRow = proxyIndex.row();
    if (proxyIndex.internalPointer()) {
        sourceRow = m_groupedSourceRows[(size_t)proxyIndex.internalPointer()-1][proxyIndex.row()];
    }

    return sourceModel()->index(sourceRow, sourceColumn);
}

int GroupProxyModel::columnCount(const QModelIndex& parent) const {
    return sourceModel()->columnCount(QModelIndex());
}

int GroupProxyModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return m_groupedSourceRows.size();
    } else {
        return m_groupedSourceRows[parent.row()].size();
    }
}

QModelIndex GroupProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column);
    } else {
        return createIndex(row, column, parent.row() + 1);
    }
}

QVariant GroupProxyModel::data(const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (!index.internalPointer()) {
        if (index.column() == 0) {
            // take from first child
            QModelIndex child = this->index(0, 0, index);
            return sourceModel()->data(mapToSource(child));
        } else {
            return QVariant();
        }
    } else {
        if (index.column() == 0) {
            return QVariant();
        } else {
            return sourceModel()->data(mapToSource(index), role);
        }
    }
}


QModelIndex GroupProxyModel::parent(const QModelIndex& child) const {
    if (!child.internalPointer()) {
        return QModelIndex();
    } else {
        return createIndex((size_t)child.internalPointer() - 1, 0);
    }
}

bool GroupProxyModel::hasChildren(const QModelIndex &parent) const {
    if (!parent.isValid() || !parent.internalPointer()) return true;
    else return false;
}

Qt::ItemFlags GroupProxyModel::flags(const QModelIndex &index) const {
    return QAbstractItemModel::flags(index);
}

void GroupProxyModel::setGroupBy(GroupBy field) {
    if (m_groupBy == field) return;
    beginResetModel();
    m_groupBy = field;
    m_groupLookup.clear();
    m_groupedSourceRows.clear();
    for (int i = 0; i < sourceModel()->rowCount(); i++) {
        QModelIndex item = sourceModel()->index(i, m_groupBy, QModelIndex());
        QVariant value = sourceModel()->data(item);
        if (!m_groupLookup.contains(value)) {
            m_groupLookup[value] = m_groupedSourceRows.size();
            m_groupedSourceRows.emplace_back();
        }
        m_groupedSourceRows[m_groupLookup[value]].push_back(i);
    }
    endResetModel();
}


