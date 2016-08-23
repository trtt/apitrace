#include "backendprofilewindow.h"
#include "groupproxymodel.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QSortFilterProxyModel>


BackendProfileWindow::BackendProfileWindow(QWidget *parent)
    : QMainWindow(parent), m_setup(false)
{
    setupUi(this);
}

BackendProfileWindow::~BackendProfileWindow()
{
	if (!m_setup) return;
    delete m_axisCPU;
    delete m_axisGPU;
    delete m_graphs;
    delete m_timelineData;
    delete m_statsTimeline;
    delete m_statsBar;
    delete m_timelineHelper;
}

void BackendProfileWindow::saveCallTableSelection(bool grouped) {
    m_callTableSelection = m_callTableSortProxy->mapSelectionToSource(callTreeView->selectionModel()->selection());
    if (grouped)
            m_callTableSelection = m_callTableGroupProxy->mapSelectionToSource(m_callTableSelection);
}

void BackendProfileWindow::restoreCallTableSelection(bool grouped) {
    if (grouped)
        m_callTableSelection = m_callTableGroupProxy->mapSelectionFromSource(m_callTableSelection);
    m_callTableSelection = m_callTableSortProxy->mapSelectionFromSource(m_callTableSelection);
    callTreeView->selectionModel()->select(m_callTableSelection, QItemSelectionModel::Select);
    if (!m_callTableSelection.indexes().empty())
        callTreeView->scrollTo(m_callTableSelection.indexes()[0]);
}

void BackendProfileWindow::setup(MetricCallDataModel* model)
{
    if (m_setup) return;

    tabWidget->setTabEnabled(0, false);

    // Proxy model for grouping
    m_callTableGroupProxy = new GroupProxyModel(model);
    m_callTableGroupProxy->setSourceModel(model);
    // Proxy model for sorting
    // Initially disable grouping
    m_callTableSortProxy = new QSortFilterProxyModel(m_callTableGroupProxy);
    m_callTableSortProxy->setSourceModel(model);
    callTreeView->setModel(m_callTableSortProxy);
    callTreeView->sortByColumn(0, Qt::AscendingOrder);

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
                    m_callTableSortProxy->setSourceModel(model);
                    restoreCallTableSelection(false);
                } else {
                    saveCallTableSelection(false);
                    m_callTableSortProxy->setSourceModel(m_callTableGroupProxy);
                }
            });

    m_axisCPU = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *model->calls().timestampHData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *model->calls().timestampLData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*model->durationDataCPU()));
    m_axisGPU = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *model->calls().timestampHData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *model->calls().timestampLData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*model->durationDataGPU()));

    for (auto& i : *model->calls().programData()) {
        auto str = QString::number(i);
        if (!m_dataFilterUnique.contains(str)) m_dataFilterUnique.append(str);
    }

    m_graphs = new MetricGraphs(*model);
    m_timelineData = new TimelineGraphData(std::make_shared<TextureBufferData<GLuint>>(*model->calls().nameHashData()),
                                            model->calls().nameHashNumEntries(),
                                            m_graphs->filter());
    m_statsTimeline = new RangeStats();
    m_statsBar = new RangeStatsMinMax();
    m_timelineHelper = new TimelineHelper(model);

    qmlRegisterType<BarGraph>("DataVis", 1, 0, "BarGraph");
    qmlRegisterType<TimelineGraph>("DataVis", 1, 0, "TimelineGraph");
    auto view = new QQuickWidget(this);
    QQmlContext *ctxt = view->rootContext();
    ctxt->setContextProperty("axisCPU", m_axisCPU);
    ctxt->setContextProperty("axisGPU", m_axisGPU);
    ctxt->setContextProperty("statsBar", m_statsBar);
    ctxt->setContextProperty("statsTimeline", m_statsTimeline);
    ctxt->setContextProperty("timelineHelper", m_timelineHelper);
    ctxt->setContextProperty("programs", QVariant::fromValue(m_dataFilterUnique));
    ctxt->setContextProperty("graphs", m_graphs);
    ctxt->setContextProperty("timelinedata", m_timelineData);
    QSurfaceFormat format(view->format());
	format.setVersion(3,2);
    view->setFormat(format);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setSource(QUrl("qrc:/main.qml"));
    graphsLayoutCall->addWidget(view);
    view->show();

    m_setup = true;
}
