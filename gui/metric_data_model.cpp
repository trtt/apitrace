#include "metric_data_model.hpp"

#include <QTextStream>
#include <QRegularExpression>

void QueryItem::addMetric(QVariant &&data) {
    m_metrics.push_back(data);
}

QVariant QueryItem::getMetric(int index) const {
    return m_metrics[index];
}


CallItem::CallItem(unsigned no, unsigned program, unsigned frame, QString name) :
    m_no(no), m_program(program), m_frame(frame),
    m_nameTableEntry(nameTable.getId(name.toStdString())) {};

unsigned CallItem::no() const {
    return m_no;
}

unsigned CallItem::program() const {
    return m_program;
}

unsigned CallItem::frame() const {
    return m_frame;
}

QString CallItem::name() const {
    return QString::fromStdString(nameTable.getString(m_nameTableEntry));
}

CallItem::StringTable<int16_t> CallItem::nameTable;


unsigned FrameItem::no() const {
    return m_no;
}


int MetricFrameDataModel::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

int MetricFrameDataModel::columnCount(const QModelIndex &parent) const
{
    return COLUMN_METRICS_BEGIN + m_metrics.size();
}

QVariant MetricFrameDataModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case COLUMN_ID:
            return "Id";
        default:
            return m_metrics[section - COLUMN_METRICS_BEGIN]->getName();
        }
    }
    else return QVariant();
}

QVariant MetricFrameDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch(index.column()) {
        case COLUMN_ID:
            return QVariant(m_data[index.row()].no());
        default:
            return m_data[index.row()].getMetric(index.column() - COLUMN_METRICS_BEGIN);
        }
    } else {
        return QVariant();
    }
}

void MetricFrameDataModel::addMetricsData(QTextStream &stream,
                                          MetricOutputLookup &lookup)
{
    QString line;

    do {
        line = stream.readLine();
    } while (line.contains(QRegularExpression("Rendered \\d+ frames")));

    QStringList tokens = line.split("\t");
    int i = 1;
    beginInsertColumns(QModelIndex(), columnCount(), columnCount() + tokens.size() - 2);
    for (auto b = lookup.begin(); b != lookup.end(); ++b) {
        for (int j = 0; j < b.value().size(); ++j) {
            MetricItem* item = lookup[b.key()][tokens.at(i)];
            m_metrics.push_back(item);
            i++;
        }
    }

    int eventId = 0;
    while (!stream.atEnd()) {
        line = stream.readLine();
        if (line.isEmpty()) break;
        tokens = line.split("\t");
        if (!init) {
            m_data.emplace_back(m_data.size()); // add new query
        }

        i = 1;
        for (auto b = lookup.begin(); b != lookup.end(); ++b) {
            for (int j = 0; j < b.value().size(); ++j) {
                const QString &token = tokens.at(i);
                if (token == QString("-")) {
                    m_data[eventId].addMetric("");
                } else {
                    MetricNumType nType = m_metrics[i-1]->getNumType();
                    switch(nType) {
                        case CNT_NUM_UINT: m_data[eventId].addMetric(token.toUInt()); break;
                        case CNT_NUM_FLOAT: m_data[eventId].addMetric(token.toFloat()); break;
                        case CNT_NUM_DOUBLE: m_data[eventId].addMetric(token.toDouble()); break;
                        case CNT_NUM_BOOL: m_data[eventId].addMetric(static_cast<QVariant>(token).toBool()); break;
                        case CNT_NUM_UINT64: m_data[eventId].addMetric(token.toULongLong()); break;
                        case CNT_NUM_INT64: m_data[eventId].addMetric(token.toLongLong()); break;
                    }
                }
                i++;
            }
        }
        eventId++;
    }
    endInsertColumns();
    if (!init) init = true;
}


int MetricCallDataModel::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

int MetricCallDataModel::columnCount(const QModelIndex &parent) const
{
    return COLUMN_METRICS_BEGIN + m_metrics.size();
}

QVariant MetricCallDataModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case COLUMN_ID:
            return "Id";
        case COLUMN_PROGRAM:
            return "Program";
        case COLUMN_FRAME:
            return "Frame";
        case COLUMN_NAME:
            return "Call";
        default:
            return m_metrics[section - COLUMN_METRICS_BEGIN]->getName();
        }
    }
    else return QVariant();
}

QVariant MetricCallDataModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch(index.column()) {
        case COLUMN_ID:
            return QVariant(m_data[index.row()].no());
        case COLUMN_PROGRAM:
            return QVariant(m_data[index.row()].program());
        case COLUMN_FRAME:
            return QVariant(m_data[index.row()].frame());
        case COLUMN_NAME:
            return QVariant(m_data[index.row()].name());
        default:
            return m_data[index.row()].getMetric(index.column() - COLUMN_METRICS_BEGIN);
        }
    } else {
        return QVariant();
    }
}

void MetricCallDataModel::addMetricsData(QTextStream &stream,
                                          MetricOutputLookup &lookup)
{
    QString line;

    do {
        line = stream.readLine();
    } while (line.contains(QRegularExpression("Rendered \\d+ frames")));

    QStringList tokens = line.split("\t");
    int i = 4;
    beginInsertColumns(QModelIndex(), columnCount(), columnCount() + tokens.size() - 5);
    for (auto b = lookup.begin(); b != lookup.end(); ++b) {
        for (int j = 0; j < b.value().size(); ++j) {
            MetricItem* item = lookup[b.key()][tokens.at(i)];
            m_metrics.push_back(item);
            i++;
        }
    }

    unsigned frame = 0;
    int eventId = 0;
    while (!stream.atEnd()) {
        line = stream.readLine();
        if (line.isEmpty()) break;
        if (line == QString("frame_end")) {
            frame++;
            continue;
        }
        tokens = line.split("\t");
        if (!init) {
            m_data.emplace_back(tokens.at(1).toUInt(), tokens.at(2).toUInt(),
                    frame, tokens.at(3)); // add new query
        }

        i = 4;
        for (auto b = lookup.begin(); b != lookup.end(); ++b) {
            for (int j = 0; j < b.value().size(); ++j) {
                const QString &token = tokens.at(i);
                if (token == QString("-")) {
                    m_data[eventId].addMetric("");
                } else {
                    MetricNumType nType = m_metrics[i-4]->getNumType();
                    switch(nType) {
                        case CNT_NUM_UINT: m_data[eventId].addMetric(token.toUInt()); break;
                        case CNT_NUM_FLOAT: m_data[eventId].addMetric(token.toFloat()); break;
                        case CNT_NUM_DOUBLE: m_data[eventId].addMetric(token.toDouble()); break;
                        case CNT_NUM_BOOL: m_data[eventId].addMetric(static_cast<QVariant>(token).toBool()); break;
                        case CNT_NUM_UINT64: m_data[eventId].addMetric(token.toULongLong()); break;
                        case CNT_NUM_INT64: m_data[eventId].addMetric(token.toLongLong()); break;
                    }
                }
                i++;
            }
        }
        eventId++;
    }
    endInsertColumns();
    if (!init) init = true;
}

#include "metric_data_model.moc"
