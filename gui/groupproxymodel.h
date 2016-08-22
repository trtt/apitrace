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

    GroupProxyModel(QObject *parent = Q_NULLPTR);

    void setSourceModel(QAbstractItemModel *sourceModel);

    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;

    int columnCount(const QModelIndex& parent) const;

    int rowCount(const QModelIndex& parent) const;

    QModelIndex index(int row, int column, const QModelIndex& parent) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QModelIndex parent(const QModelIndex& child) const;

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    void setGroupBy(GroupBy field);

private:
    GroupBy m_groupBy;
    QHash<QVariant, int> m_groupLookup;
    std::vector<std::vector<int>> m_groupedSourceRows;
};
