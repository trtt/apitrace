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
