#pragma once

#include "metric_data_model.hpp"
#include "bargraph.h"

class MetricGraphs : public QAbstractListModel
{
    Q_OBJECT

public:
    enum CustomRoles {
        CaptionRole = Qt::UserRole + 1,
        DataRole = Qt::UserRole + 2,
        SizeRole
    };

    explicit MetricGraphs(MetricCallDataModel& model);
    ~MetricGraphs();

    std::shared_ptr<TextureBufferData<GLuint>> filter() { return m_filter; }


    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const;

private:
    QString name(unsigned index) const;
    void addGraphsData();

    MetricCallDataModel& m_model;
    std::shared_ptr<TextureBufferData<GLuint>> m_filter;
    std::vector<std::shared_ptr<TextureBufferData<GLfloat>>> m_textureData;
    std::vector<BarGraphData*> m_data;
    std::vector<MetricItem*> m_metrics;
};
