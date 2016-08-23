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
