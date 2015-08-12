#include <QAbstractItemModel>
#include <QModelIndex>
#include <QList>
#include <QSet>
#include <QVariant>
#include <QIODevice>

class GroupItem;
class MetricItem;

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
                        const QString desc, GroupItem* parentItem)
        : m_id(id), m_name(name), m_desc(desc), m_parentItem(parentItem) {};
    ~MetricItem() = default;

    int getId() const;
    QString getName() const;
    QString getDesc() const;
    GroupItem* parentItem();

private:
    int m_id;
    QString m_name, m_desc;
    GroupItem* m_parentItem;
};

class MetricSelectionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum MetricSelectionRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        DescRole,
        MindexRole,
        CheckRole
    };

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
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    void generateMetricList() const;

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
    QHash<QModelIndex, int> childNodesSelected;
};
