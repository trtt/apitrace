#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QMainWindow>
#include <QTableView>

#include <QQuickWidget>
#include <QQmlContext>

#include "metric_selection_model.hpp"
#include "metric_data_model.hpp"
#include "metric_graph_data.hpp"
#include "graphing/histogramview.h"
#include "bargraph.h"
#include "timelinegraph.h"
#include "rangestats.h"
#include "metric_graphs.h"

#include "ui_metric_selection.h"
#include "ui_backendprofile.h"

Ui::MetricSelection ui;
Ui::BackendProfileWindow winui;
//MetricFrameDataModel modelFrame;
MetricCallDataModel modelCall;

TimelineAxis* axisCPU;
TimelineAxis* axisGPU;
RangeStats* stats;
TimelineGraphData* timelineData;
QStringList dataFilterUnique;

void addMetrics(MetricSelectionModel& model, const char* target) {
    QDialog* widget = new QDialog;
    ui.setupUi(widget);
    ui.treeView->setModel(&model);
    ui.treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (!widget->exec()) return;

    QString cliOptionFrame, cliOptionCall;
    model.generateMetricList(cliOptionFrame, cliOptionCall);

    QProcess process;
    QString prog = QLatin1String("./glretrace");
    QStringList arg = QStringList();
    arg << cliOptionFrame;
    arg << cliOptionCall;
    arg << target;
    process.start(prog,
            arg,
            QIODevice::ReadOnly);
    process.setReadChannel(QProcess::StandardOutput);
    process.waitForFinished(-1);
    QTextStream stream(&process);
    //if (!mFrame.empty()) {
        //modelFrame.addMetricsData(stream, mFrame);
    //}
    if (!model.selectedForCalls().empty()) {
        modelCall.addMetricsData(stream, model.selectedForCalls());
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QProcess process;
    QString prog = QLatin1String("./glretrace");
    QStringList arg;
    arg << QLatin1String("--list-metrics");
    arg << argv[1];
    process.start(prog,
                  arg,
                  QIODevice::ReadOnly);
    process.setReadChannel(QProcess::StandardOutput);
    process.waitForFinished(-1);
    MetricSelectionModel model(process);
    process.kill();

    // metrics selection dialog at the start
    addMetrics(model, argv[1]);

    QMainWindow win;
    winui.setupUi(&win);
    //winui.frameTableView->setModel(&modelFrame);
    winui.callTableView->setModel(&modelCall);

    // Add metrics buttons
    //win.connect(winui.frameAddMetrics, &QPushButton::clicked, [&]{
            //addMetrics(model, argv[1]);
    //});
    win.connect(winui.callAddMetrics, &QPushButton::clicked, [&]{
            addMetrics(model, argv[1]);
    });

    // Frame and call graphs
    //HistogramView* histFrame = new HistogramView(0);
    //MetricGraphData dataFrame = MetricGraphData(&modelFrame, 1);
    //histFrame->setDataProvider(&dataFrame);
    //histFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //winui.graphsLayoutFrame->addWidget(histFrame);

    //HistogramView* histCall = new HistogramView(0);
    //MetricGraphData dataCall = MetricGraphData(&modelCall, 4);
    //histCall->setDataProvider(&dataCall);
    //histCall->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //winui.graphsLayoutCall->addWidget(histCall);

    axisCPU = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *modelCall.calls().timestampHData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *modelCall.calls().timestampLData(DrawcallStorage::TimestampCPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*modelCall.durationDataCPU()));
    axisGPU = new TimelineAxis(std::make_shared<TextureBufferData<GLuint>>(
                *modelCall.calls().timestampHData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLuint>>(
                *modelCall.calls().timestampLData(DrawcallStorage::TimestampGPU)),
            std::make_shared<TextureBufferData<GLfloat>>(*modelCall.durationDataGPU()));

    for (auto& i : *modelCall.calls().programData()) {
        auto str = QString::number(i);
        if (!dataFilterUnique.contains(str)) dataFilterUnique.append(str);
    }

    MetricGraphs graphs(modelCall);
    timelineData = new TimelineGraphData(std::make_shared<TextureBufferData<GLuint>>(*modelCall.calls().nameHashData()),
                                            modelCall.calls().nameHashNumEntries(),
                                            graphs.filter());
    stats = new RangeStats();

    qmlRegisterType<BarGraph>("DataVis", 1, 0, "BarGraph");
    qmlRegisterType<TimelineGraph>("DataVis", 1, 0, "TimelineGraph");
    QQuickWidget view;
    QQmlContext *ctxt = view.rootContext();
    ctxt->setContextProperty("axisCPU", axisCPU);
    ctxt->setContextProperty("axisGPU", axisGPU);
    ctxt->setContextProperty("stats", stats);
    ctxt->setContextProperty("programs", QVariant::fromValue(dataFilterUnique));
    ctxt->setContextProperty("graphs", &graphs);
    ctxt->setContextProperty("timelinedata", timelineData);
    QSurfaceFormat format(view.format());
    format.setVersion(3,1);
    view.setFormat(format);
    view.setResizeMode(QQuickWidget::SizeRootObjectToView);
    view.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view.setSource(QUrl("../gui/main.qml"));
    winui.graphsLayoutCall->addWidget(&view);
    view.show();

    // Selected column is plotted
    //win.connect(winui.frameTableView->selectionModel(),
                 //&QItemSelectionModel::selectionChanged, [&] {
        //QList<QModelIndex> columns = winui.frameTableView->selectionModel()->selectedColumns();
        //if (!columns.empty()) {
            //dataFrame.setColumn(columns[0].column());
            //histFrame->update();
        //}
    //});
    //win.connect(winui.callTableView->selectionModel(),
                 //&QItemSelectionModel::selectionChanged, [&] {
        //QList<QModelIndex> columns = winui.callTableView->selectionModel()->selectedColumns();
        //if (!columns.empty()) {
            //dataCall.setColumn(columns[0].column());
            //histCall->update();
        //}
    //});

    win.show();
    return app.exec();

}
