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

#include "ui_backendprofile.h"

#include "metric_selection_model.hpp"
#include "metric_data_model.hpp"
#include "metric_graph_data.hpp"
#include "timelinegraph.h"
#include "rangestats.h"
#include "metric_graphs.h"

class GroupProxyModel;
class QSortFilterProxyModel;

class BackendProfileWindow : public QMainWindow, public Ui_BackendProfileWindow
{
    Q_OBJECT

public:
    BackendProfileWindow(QWidget* parent = 0);
    ~BackendProfileWindow();
    void setup(MetricCallDataModel* callModel, MetricFrameDataModel* frameModel);
    bool isSetuped() const { return m_setup; }
    void updateViews();

private:
    void setupCallView();
    void setupFrameView();

    void saveCallTableSelection(bool grouped = true);
    void restoreCallTableSelection(bool grouped = true);

    bool m_setup;
    TimelineAxis* m_axisCPU;
    TimelineAxis* m_axisGPU;
    MetricGraphs* m_graphs;
    TimelineGraphData* m_timelineData;
    RangeStats* m_statsTimeline;
    RangeStatsMinMax* m_statsBar;
    TimelineHelper* m_timelineHelper;
    QStringList m_dataFilterUnique;
    GroupProxyModel* m_callTableGroupProxy;
    QSortFilterProxyModel* m_callTableSortProxy;
    QItemSelection m_callTableSelection;
};
