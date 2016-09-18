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

    explicit MetricGraphs(const std::vector<MetricStorage>& modelMetrics,
                          std::shared_ptr<TextureBufferData<GLuint>> filter = nullptr);
    ~MetricGraphs();

    std::shared_ptr<TextureBufferData<GLuint>> filter() { return m_filter; }


    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    void addGraphsData();
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const;

private:
    QString name(unsigned index) const;

    const std::vector<MetricStorage>& m_modelMetrics;
    std::shared_ptr<TextureBufferData<GLuint>> m_filter;
    std::vector<std::shared_ptr<TextureBufferData<GLfloat>>> m_textureData;
    std::vector<BarGraphData*> m_data;
    std::vector<MetricItem*> m_metrics;
};
