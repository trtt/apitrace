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

#include <QAbstractProxyModel>
#include <QHash>

class GroupProxyModel : public QAbstractProxyModel
{
    Q_OBJECT

public:

    enum GroupBy {
        GROUP_BY_PROGRAM = 1,
        GROUP_BY_FRAME   = 2,
        GROUP_BY_CALL    = 3,
        GROUP_BY_INVALID
    };

    enum StatsOperation {
        OP_SUM,
        OP_MAX,
        OP_MIN,
        OP_AVG,
        OP_INVALID
    };

    GroupProxyModel(QObject *parent = Q_NULLPTR);

    void setSourceModel(QAbstractItemModel *sourceModel);

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    int columnCount(const QModelIndex& parent) const;

    int rowCount(const QModelIndex& parent) const;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex parent(const QModelIndex& child) const;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setGroupBy(GroupBy field);

    void addStats(StatsOperation op);

public slots:
    void beginInsertColumnsSlot(const QModelIndex &parent, int first, int last);

    void endInsertColumnsSlot();

private:
    GroupBy m_groupBy;
    StatsOperation m_statsOp;
    QHash<QVariant, int> m_groupLookup;
    std::vector<std::vector<int>> m_groupedSourceRows;

    QVariant stats(const QModelIndex &index) const;
};
