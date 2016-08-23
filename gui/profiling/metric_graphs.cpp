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
