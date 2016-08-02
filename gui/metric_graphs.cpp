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
    beginInsertRows(QModelIndex(), m_data.size(), m_model.metrics().size()-1);
    for (int i = m_data.size(); i < m_model.metrics().size(); i++) {
        auto& m = m_model.metrics()[i];
        m_textureData.push_back(std::make_shared<TextureBufferData<GLfloat>>(*m.vector()));
        m_data.push_back(new BarGraphData(m_textureData.back(), m_filter));
    }
    endInsertRows();
}

int MetricGraphs::rowCount(const QModelIndex &parent) const
{
    return m_data.size();
}

QVariant MetricGraphs::data(const QModelIndex &index, int role) const
{
    if (role == CaptionRole) {
        return name(index.row());
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
