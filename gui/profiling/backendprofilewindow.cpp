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

#include "backendprofilewindow.h"
#include "groupproxymodel.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QSortFilterProxyModel>


BackendProfileWindow::BackendProfileWindow(QWidget *parent)
    : QMainWindow(parent), m_setup(false), m_callModel(nullptr), m_frameModel(nullptr)
{
    setupUi(this);
    for (int i = 0; i < VIEWS_NUM; i++) {
        m_axisCPU[i] = nullptr;
        m_axisGPU[i] = nullptr;
        m_graphs[i] = nullptr;
        m_timelineData[i] = nullptr;
        m_statsTimeline[i] = nullptr;
        m_statsBar[i] = nullptr;
        tabWidget->setTabEnabled(i, false);
        m_scroll[i] = nullptr;
    }
    qmlRegisterType<BarGraph>("DataVis", 1, 0, "BarGraph");
    qmlRegisterType<TimelineGraph>("DataVis", 1, 0, "TimelineGraph");
}

BackendProfileWindow::~BackendProfileWindow()
{
	if (!m_setup) return;
    for (int i = 0; i < VIEWS_NUM; i++) {
        delete m_axisCPU[i];
        delete m_axisGPU[i];
        delete m_graphs[i];
        delete m_timelineData[i];
        delete m_statsTimeline[i];
        delete m_statsBar[i];
    }
    delete m_timelineHelper;
}

void BackendProfileWindow::saveCallTableSelection(bool grouped) {
    m_callTableSelection = m_metricTableSortProxy->mapSelectionToSource(metricTreeView->selectionModel()->selection());
    if (grouped)
            m_callTableSelection = m_callTableGroupProxy->mapSelectionToSource(m_callTableSelection);
}

void BackendProfileWindow::restoreCallTableSelection(bool grouped) {
    if (grouped)
        m_callTableSelection = m_callTableGroupProxy->mapSelectionFromSource(m_callTableSelection);
    m_callTableSelection = m_metricTableSortProxy->mapSelectionFromSource(m_callTableSelection);
    metricTreeView->selectionModel()->select(m_callTableSelection, QItemSelectionModel::Select);
    if (!m_callTableSelection.indexes().empty())
        metricTreeView->scrollTo(m_callTableSelection.indexes()[0]);
}

void BackendProfileWindow::tabChange(int tab) {
    bool floating = dockWidget->isFloating();
    if (tab == 0) { // Change to frame tab
        frameGraphTab->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
        groupBox->setEnabled(false);
        statsComboBox->setEnabled(false);
        metricTreeView->setColumnsFrozen(MetricFrameDataModel::COLUMN_METRICS_BEGIN);
        m_metricTableSortProxy->setSourceModel(m_frameModel);
        // sync displayed time range
        if (m_axisCPU[CALL_VIEW]) {
            m_scroll[FRAME_VIEW]->setProperty("position",
                        m_scroll[CALL_VIEW]->property("position"));
            m_scroll[FRAME_VIEW]->setProperty("size",
                        m_scroll[CALL_VIEW]->property("size"));
        }
    } else { // Change to call tab
        callGraphTab->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
        groupBox->setEnabled(true);
        metricTreeView->setColumnsFrozen(MetricCallDataModel::COLUMN_METRICS_BEGIN);
        if (none_radio->isChecked()) {
            m_metricTableSortProxy->setSourceModel(m_callModel);
        } else {
            m_metricTableSortProxy->setSourceModel(m_callTableGroupProxy);
            statsComboBox->setEnabled(true);
        }
        // sync displayed time range
        if (m_axisCPU[FRAME_VIEW]) {
            m_scroll[CALL_VIEW]->setProperty("position",
                        m_scroll[FRAME_VIEW]->property("position"));
            m_scroll[CALL_VIEW]->setProperty("size",
                        m_scroll[FRAME_VIEW]->property("size"));
        }
    }
    if (floating) dockWidget->setFloating(true);
}

void BackendProfileWindow::focusEvent(int id) {
    // scroll to and select event in the table view
    QModelIndex index;
    if (tabWidget->currentIndex() == FRAME_VIEW) {
        // frames in table
        index = m_metricTableSortProxy->mapFromSource(m_frameModel->index(id, 0));
    } else if (none_radio->isChecked()) {
        // calls (raw) in table
        index = m_metricTableSortProxy->mapFromSource(m_callModel->index(id, 0));
    } else {
        // calls (grouped) in table
        index = m_callTableGroupProxy->mapFromSource(m_callModel->index(id, 0));
        index = m_metricTableSortProxy->mapFromSource(index);
    }
    metricTreeView->scrollTo(index);
    metricTreeView->selectionModel()->select(index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void BackendProfileWindow::setup(MetricCallDataModel* callModel,
                                 MetricFrameDataModel* frameModel)
{
    if (!m_callModel && callModel) setupCallView(callModel);
    if (!m_frameModel && frameModel) setupFrameView(frameModel);
    if (m_setup) return;

    // Proxy model for sorting
    m_metricTableSortProxy = new QSortFilterProxyModel(this);
    metricTreeView->setModel(m_metricTableSortProxy);
    metricTreeView->setUniformRowHeights(true);
    metricTreeView->sortByColumn(0, Qt::AscendingOrder);

    connect(tabWidget, &QTabWidget::currentChanged,
            this, &BackendProfileWindow::tabChange);

    tabWidget->setCurrentIndex(m_frameModel ? FRAME_VIEW : CALL_VIEW);
    tabChange(tabWidget->currentIndex()); //force

    // Handle grouping mode switches (save/restore selection, change models)
    connect(call_radio, &QRadioButton::toggled, [=](bool b) {
                if (b) {
                    m_callTableGroupProxy->setGroupBy(GroupProxyModel::GROUP_BY_CALL);
                    restoreCallTableSelection(true);
                } else {
                    saveCallTableSelection(true);
                }
            });
    connect(frame_radio, &QRadioButton::toggled, [=](bool b) {
                if (b) {
                    m_callTableGroupProxy->setGroupBy(GroupProxyModel::GROUP_BY_FRAME);
                    restoreCallTableSelection(true);
                } else {
                    saveCallTableSelection(true);
                }
            });
    connect(program_radio, &QRadioButton::toggled, [=](bool b) {
                if (b) {
                    m_callTableGroupProxy->setGroupBy(GroupProxyModel::GROUP_BY_PROGRAM);
                    restoreCallTableSelection(true);
                } else {
                    saveCallTableSelection(true);
                }
            });
    connect(none_radio, &QRadioButton::toggled, [=](bool b) {
                if (b) {
                    m_metricTableSortProxy->setSourceModel(m_callModel);
                    restoreCallTableSelection(false);
                    statsComboBox->setEnabled(false);
                } else {
                    saveCallTableSelection(false);
                    m_metricTableSortProxy->setSourceModel(m_callTableGroupProxy);
                    statsComboBox->setEnabled(true);
                }
            });

    connect(statsComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [=](int index) {
                m_callTableGroupProxy->addStats(
                        static_cast<GroupProxyModel::StatsOperation>(index)
                );
            });

    m_setup = true;
}

void BackendProfileWindow::setupCallView(MetricCallDataModel* callModel)
{
    m_callModel = callModel;

    m_axisCPU[CALL_VIEW] = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *callModel->calls().timestampHData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *callModel->calls().timestampLData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*callModel->durationDataCPU()));
    m_axisGPU[CALL_VIEW] = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *callModel->calls().timestampHData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *callModel->calls().timestampLData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*callModel->durationDataGPU()));

    for (auto& i : *callModel->calls().programData()) {
        auto str = QString::number(i);
        if (!m_dataFilterUnique.contains(str)) m_dataFilterUnique.append(str);
    }

    m_graphs[CALL_VIEW] = new MetricGraphs(callModel->metrics(),
        std::make_shared<TextureBufferData<GLuint>>(*callModel->calls().programData()));
    connect(callModel, &QAbstractItemModel::columnsInserted,
            m_graphs[CALL_VIEW], &MetricGraphs::addGraphsData);

    m_timelineData[CALL_VIEW] = new TimelineGraphData(std::make_shared<TextureBufferData<GLuint>>(*callModel->calls().nameHashData()),
                                            callModel->calls().nameHashNumEntries(),
                                            m_graphs[CALL_VIEW]->filter());
    m_statsTimeline[CALL_VIEW] = new RangeStats();
    m_statsBar[CALL_VIEW] = new RangeStatsMinMax();
    m_timelineHelper = new TimelineHelper(callModel);

    auto view = new QQuickWidget(this);
    QQmlContext *ctxt = view->rootContext();
    ctxt->setContextProperty("viewType", CALL_VIEW);
    ctxt->setContextProperty("axisCPU", m_axisCPU[CALL_VIEW]);
    ctxt->setContextProperty("axisGPU", m_axisGPU[CALL_VIEW]);
    ctxt->setContextProperty("statsBar", m_statsBar[CALL_VIEW]);
    ctxt->setContextProperty("statsTimeline", m_statsTimeline[CALL_VIEW]);
    ctxt->setContextProperty("timelineHelper", m_timelineHelper);
    ctxt->setContextProperty("programs", QVariant::fromValue(m_dataFilterUnique));
    ctxt->setContextProperty("graphs", m_graphs[CALL_VIEW]);
    ctxt->setContextProperty("timelinedata", m_timelineData[CALL_VIEW]);
    QSurfaceFormat format(view->format());
    format.setVersion(3,2);
    view->setFormat(format);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setSource(QUrl("qrc:/profiling/qml/main.qml"));
    graphsLayoutCall->addWidget(view);
    m_scroll[CALL_VIEW] = view->rootObject()->findChild<QObject*>("scroll");
    view->show();

    // Proxy model for grouping
    // Initially grouping is disabled
    m_callTableGroupProxy = new GroupProxyModel(callModel);
    m_callTableGroupProxy->setSourceModel(callModel);

    connect(view->rootObject(), SIGNAL(eventDoubleClicked(int)),
            this, SLOT(focusEvent(int)));

    tabWidget->setTabEnabled(CALL_VIEW, true);
}

void BackendProfileWindow::setupFrameView(MetricFrameDataModel* frameModel)
{
    m_frameModel = frameModel;

    m_axisCPU[FRAME_VIEW] = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *frameModel->frames().timestampHData(FrameStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *frameModel->frames().timestampLData(FrameStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*frameModel->durationDataCPU()));
    m_axisGPU[FRAME_VIEW] = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *frameModel->frames().timestampHData(FrameStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *frameModel->frames().timestampLData(FrameStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*frameModel->durationDataGPU()));

    m_graphs[FRAME_VIEW] = new MetricGraphs(frameModel->metrics());
    connect(frameModel, &QAbstractItemModel::columnsInserted,
            m_graphs[FRAME_VIEW], &MetricGraphs::addGraphsData);

    m_timelineData[FRAME_VIEW] = new TimelineGraphData(nullptr, 0, nullptr);
    m_statsTimeline[FRAME_VIEW] = new RangeStats();
    m_statsBar[FRAME_VIEW] = new RangeStatsMinMax();

    auto view = new QQuickWidget(this);
    QQmlContext *ctxt = view->rootContext();
    ctxt->setContextProperty("viewType", FRAME_VIEW);
    ctxt->setContextProperty("axisCPU", m_axisCPU[FRAME_VIEW]);
    ctxt->setContextProperty("axisGPU", m_axisGPU[FRAME_VIEW]);
    ctxt->setContextProperty("statsBar", m_statsBar[FRAME_VIEW]);
    ctxt->setContextProperty("statsTimeline", m_statsTimeline[FRAME_VIEW]);
    ctxt->setContextProperty("graphs", m_graphs[FRAME_VIEW]);
    ctxt->setContextProperty("timelinedata", m_timelineData[FRAME_VIEW]);
    QSurfaceFormat format(view->format());
    format.setVersion(3,2);
    view->setFormat(format);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setSource(QUrl("qrc:/profiling/qml/main.qml"));
    graphsLayoutFrame->addWidget(view);
    m_scroll[FRAME_VIEW] = view->rootObject()->findChild<QObject*>("scroll");
    view->show();

    connect(view->rootObject(), SIGNAL(eventDoubleClicked(int)),
            this, SLOT(focusEvent(int)));

    tabWidget->setTabEnabled(FRAME_VIEW, true);
}
