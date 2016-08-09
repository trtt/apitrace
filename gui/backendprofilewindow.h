#pragma once

#include "ui_backendprofile.h"

#include "metric_selection_model.hpp"
#include "metric_data_model.hpp"
#include "metric_graph_data.hpp"
#include "timelinegraph.h"
#include "rangestats.h"
#include "metric_graphs.h"


class BackendProfileWindow : public QMainWindow, public Ui_BackendProfileWindow
{
    Q_OBJECT

public:
    BackendProfileWindow(QWidget* parent = 0);
    ~BackendProfileWindow();
    void setup(MetricCallDataModel* model);
    bool isSetuped() const { return m_setup; }

public slots:

private:
    bool m_setup;
    TimelineAxis* m_axisCPU;
    TimelineAxis* m_axisGPU;
    MetricGraphs* m_graphs;
    TimelineGraphData* m_timelineData;
    RangeStats* m_statsTimeline;
    RangeStatsMinMax* m_statsBar;
    TimelineHelper* m_timelineHelper;
    QStringList m_dataFilterUnique;
};
