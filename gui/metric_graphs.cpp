#include "metric_graphs.h"

MetricGraphs::MetricGraphs(MetricCallDataModel& model) : m_model(model) {
    m_filter = std::make_shared<TextureBufferData<GLuint>>(*model.calls().programData());
    addGraphsData();
    connect(&model, &QAbstractItemModel::columnsInserted, this, &MetricGraphs::addGraphsData);
}

MetricGraphs::~MetricGraphs() {
    for (auto& g : m_data) {
        delete g;
    }
}

void MetricGraphs::addGraphsData() {
    auto oldSize = m_data.size();
    for (int i = (!oldSize) ? 0 : oldSize+2; i < m_model.metrics().size(); i++) {
        auto& m = m_model.metrics()[i];
        if ((m.metric()->getName() != QLatin1String("CPU Start")) &&
            (m.metric()->getName() != QLatin1String("GPU Start")))
        {
            m_textureData.push_back(std::make_shared<TextureBufferData<GLfloat>>(*m.vector()));
            m_data.push_back(new BarGraphData(m_textureData.back(), m_filter));
            m_metrics.push_back(m.metric());
        }
    }
    beginInsertRows(QModelIndex(), oldSize, m_data.size()-1);
    endInsertRows();
}

int MetricGraphs::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

QVariant MetricGraphs::data(const QModelIndex &index, int role) const
{
    if (role == CaptionRole) {
        return m_metrics[index.row()]->getName();
    } else if (role == DataRole) {
        return QVariant::fromValue(m_data[index.row()]);
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> MetricGraphs::roleNames() const
{
    auto roles = QAbstractItemModel::roleNames();
    roles[CaptionRole] = "bgcaption";
    roles[DataRole] = "bgdata";
    return roles;
}

QString MetricGraphs::name(unsigned index) const {
    return m_model.metrics()[index].metric()->getName();
}
