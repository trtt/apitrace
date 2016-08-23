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

#include "groupproxymodel.h"
#include <algorithm>

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
    if (this->sourceModel())
        this->sourceModel()->disconnect(this);
    QAbstractProxyModel::setSourceModel(sourceModel);
    setGroupBy(GROUP_BY_FRAME);
    connect(sourceModel, &QAbstractItemModel::columnsAboutToBeInserted,
            this, &GroupProxyModel::beginInsertColumnsSlot);
    connect(sourceModel, &QAbstractItemModel::columnsInserted,
            this, &GroupProxyModel::endInsertColumnsSlot);
}

void GroupProxyModel::beginInsertColumnsSlot(const QModelIndex &parent, int first, int last)
{
    layoutAboutToBeChanged();
    beginInsertColumns(QModelIndex(), first, last);
}

void GroupProxyModel::endInsertColumnsSlot() {
    endInsertColumns();
    layoutChanged();
}

QModelIndex GroupProxyModel::mapFromSource(const QModelIndex &sourceIndex) const {
    QVariant group = sourceModel()->data(sourceModel()->index(sourceIndex.row(),
                                         m_groupBy, QModelIndex()));
    int parentRow = m_groupLookup[group];
    const QModelIndex parent = index(parentRow, 0, QModelIndex());
    const std::vector<int>& parentRows = m_groupedSourceRows[parentRow];
    int rowInParent = std::distance(parentRows.begin(),
                        std::equal_range(parentRows.begin(), parentRows.end(),
                                         sourceIndex.row()).first);
    return index(rowInParent, sourceIndex.column(), parent);
}

QModelIndex GroupProxyModel::mapToSource(const QModelIndex &proxyIndex) const {
    if (!sourceModel() || !proxyIndex.isValid())
        return QModelIndex();
    if (!proxyIndex.internalPointer())
        return QModelIndex();

    int sourceColumn = proxyIndex.column();
    if (proxyIndex.column() == 0) {
        sourceColumn = m_groupBy;
    } else if (proxyIndex.column() < m_groupBy + 1) {
        sourceColumn--;
    }

    int sourceRow = proxyIndex.row();
    sourceRow = m_groupedSourceRows[(size_t)proxyIndex.internalPointer()-1][proxyIndex.row()];

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

QVariant GroupProxyModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const
{
    int sourceColumn = section;
    if (section == 0) {
        sourceColumn = m_groupBy;
    } else if (section < m_groupBy + 1) {
        sourceColumn--;
    }
    return sourceModel()->headerData(sourceColumn, orientation, role);
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


