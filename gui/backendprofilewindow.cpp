#include "backendprofilewindow.h"

#include <QQuickWidget>
#include <QQmlContext>


BackendProfileWindow::BackendProfileWindow(QWidget *parent)
    : QMainWindow(parent), m_setup(false)
{
    setupUi(this);
}

BackendProfileWindow::~BackendProfileWindow()
{
	if (!m_setup) return;
    delete m_axisCPU;
    delete m_graphs;
    delete m_timelineData;
    delete m_statsTimeline;
    delete m_statsBar;
    delete m_timelineHelper;
}

void BackendProfileWindow::setup(MetricCallDataModel* model)
{
    if (m_setup) return;

    tabWidget->setTabEnabled(0, false);

    callTableView->setModel(model);

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
