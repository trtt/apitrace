#pragma once

#include "graphing/graphing.h"
#include "metric_data_model.hpp"

class MetricGraphData : public GraphDataProvider
{
public:
    MetricGraphData(QAbstractTableModel* model, int column) : model(model),
                                                              column(column) {};

    qint64 size() const {
        return model->rowCount();
    }

    qint64 value(qint64 index) const {
        return model->data(model->index(static_cast<int>(index), column), Qt::DisplayRole).toULongLong();
    }

    bool selected(qint64 index) const {return false;}

    QString itemTooltip(qint64 index) const {
        return QString("Id: ") + model->data(model->index(static_cast<int>(index), 0), Qt::DisplayRole).toString();
    }

    void itemDoubleClicked(qint64 index) const {}

    void setSelectionState(SelectionState* state) {}

    void setColumn(int column) {this->column = column;}

private:
    QAbstractTableModel* model;
    int column;
};
