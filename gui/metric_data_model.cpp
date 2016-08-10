#include "metric_data_model.hpp"
#include <iostream>
#include <QDebug>

#include <QTextStream>
#include <QRegularExpression>


void MetricStorage::addMetricData(float data) {
    this->data->push_back(data);
}


void DrawcallStorage::addDrawcall(unsigned no, unsigned program, unsigned frame, QString name)
{
    s_no.push_back(no);
    s_program.push_back(program);
    s_frame.push_back(frame);
    nameHash.push_back(nameTable.getId(name.toLatin1().data()));
}

void DrawcallStorage::addTimestamp(qlonglong time, TimestampType type) {
    static qlonglong first;
    if (!first && type == TimestampCPU) first = time;
    s_timestampH[type].push_back((time - first) >> 32); // GPU Start (32 bit high)
    s_timestampL[type].push_back(time - first); // GPU Start (32 bit low)
}

qlonglong DrawcallStorage::getTimestamp(unsigned index, TimestampType type) const {
    return ((qlonglong)s_timestampH[type][index] << 32) + s_timestampL[type][index];
}


int MetricCallDataModel::rowCount(const QModelIndex &parent) const
{
    return m_calls.size();
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
            return "Call ID";
        case COLUMN_PROGRAM:
            return "Program";
        case COLUMN_FRAME:
            return "Frame";
        case COLUMN_NAME:
            return "Call";
        default:
            return m_metrics[section - COLUMN_METRICS_BEGIN].metric()->getName();
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
            return QVariant(m_calls.no(index.row()));
        case COLUMN_PROGRAM:
            return QVariant(m_calls.program(index.row()));
        case COLUMN_FRAME:
            return QVariant(m_calls.frame(index.row()));
        case COLUMN_NAME:
            return QString::fromStdString(m_calls.name(index.row()));
        default:
            auto name = m_metrics[index.column() - COLUMN_METRICS_BEGIN].metric()->getName();
            if (name == QLatin1String("CPU Start")) {
                return 1e-9 * m_calls.getTimestamp(index.row(), DrawcallStorage::TimestampCPU);
            } else if (name == QLatin1String("GPU Start")) {
                return 1e-9 * m_calls.getTimestamp(index.row(), DrawcallStorage::TimestampGPU);
            } else {
                return m_metrics[index.column() - COLUMN_METRICS_BEGIN].getMetricData(index.row());
            }
        }
    } else {
        return QVariant();
    }
}

void MetricCallDataModel::addMetricsData(QTextStream &stream,
                                         const MetricOutputLookup &lookup)
{
    QString line;
    unsigned oldMetricsSize = m_metrics.size();

    do {
        line = stream.readLine();
    } while (line.contains(QRegularExpression("Rendered \\d+ frames")));

    QStringList tokens = line.split("\t");
    int i = 4;
    beginInsertColumns(QModelIndex(), columnCount(), columnCount() + tokens.size() - 5);
    // returned order of backends should be sorted (same as requested)
    for (auto b = lookup.begin(); b != lookup.end(); ++b) {
        for (int j = 0; j < b.value().size(); ++j) {
            MetricItem* item = lookup[b.key()][tokens.at(i)];
            m_metrics.emplace_back(item);
            if (item->getName() == QLatin1String("CPU Duration")) {
                durationCPUIndexInMetrics = m_metrics.size() - 1;
            } else if (item->getName() == QLatin1String("GPU Duration")) {
                durationGPUIndexInMetrics = m_metrics.size() - 1;
            }
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
            m_calls.addDrawcall(tokens.at(1).toUInt(), tokens.at(2).toUInt(),
                                frame, tokens.at(3)); // add new query
        }

        i = 4 + oldMetricsSize;
        for (auto b = lookup.begin(); b != lookup.end(); ++b) {
            for (int j = 0; j < b.value().size(); ++j) {
                const QString &token = tokens.at(i-oldMetricsSize);
                if (m_metrics[i-4].metric()->getName() == QLatin1String("CPU Start")) {
                    m_calls.addTimestamp(token.toLongLong(), DrawcallStorage::TimestampCPU);
                } else if (m_metrics[i-4].metric()->getName() == QLatin1String("GPU Start")) {
                    m_calls.addTimestamp(token.toLongLong(), DrawcallStorage::TimestampGPU);
                }
                else if (token == QString("-")) {
                    m_metrics[i-4].addMetricData(0);
                } else {
                    MetricNumType nType = m_metrics[i-4].metric()->getNumType();
                    double mult = 1.;
                    if ((m_metrics[i-4].metric()->getName() == QLatin1String("CPU Duration"))
                        || (m_metrics[i-4].metric()->getName() == QLatin1String("GPU Duration"))) {
                        mult = 1e-9;
                    }
                    switch(nType) {
                        case CNT_NUM_UINT: m_metrics[i-4].addMetricData(token.toUInt()); break;
                        case CNT_NUM_FLOAT: m_metrics[i-4].addMetricData(token.toFloat()); break;
                        case CNT_NUM_DOUBLE: m_metrics[i-4].addMetricData(token.toDouble()); break;
                        case CNT_NUM_BOOL: m_metrics[i-4].addMetricData(static_cast<QVariant>(token).toBool()); break;
                        case CNT_NUM_UINT64: m_metrics[i-4].addMetricData(token.toULongLong() * mult); break;
                        case CNT_NUM_INT64: m_metrics[i-4].addMetricData(token.toLongLong() * mult); break;
                    }
                    if (m_metrics[i-4].metric()->getName() == QLatin1String("GPU Duration")) {
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

const std::vector<float>* MetricCallDataModel::durationDataCPU() const {
    return m_metrics[durationCPUIndexInMetrics].vector();
}

const std::vector<float>* MetricCallDataModel::durationDataGPU() const {
    return m_metrics[durationGPUIndexInMetrics].vector();
}

#include "metric_data_model.moc"
