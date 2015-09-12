#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QMainWindow>
#include <QTableView>

#include "metric_selection_model.hpp"
#include "metric_data_model.hpp"
#include "metric_graph_data.hpp"
#include "graphing/histogramview.h"

#include "ui_metric_selection.h"
#include "ui_qqmain.h"

Ui::Dialog ui;
Ui::MainWindow winui;
MetricFrameDataModel modelFrame;
MetricCallDataModel modelCall;

void addMetrics(MetricSelectionModel& model, const char* target) {
    QDialog* widget = new QDialog;
    ui.setupUi(widget);
    ui.treeView->setModel(&model);
    ui.treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (!widget->exec()) return;

    QString cliOptionFrame, cliOptionCall;
    QHash<QString, QHash<QString, MetricItem*>> mFrame;
    QHash<QString, QHash<QString, MetricItem*>> mCall;
    model.generateMetricList(cliOptionFrame, cliOptionCall, mFrame, mCall);

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
    if (!mFrame.empty()) {
        modelFrame.addMetricsData(stream, mFrame);
    }
    if (!mCall.empty()) {
        modelCall.addMetricsData(stream, mCall);
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
    winui.frameTableView->setModel(&modelFrame);
    winui.callTableView->setModel(&modelCall);

    // Add metrics buttons
    win.connect(winui.frameAddMetrics, &QPushButton::clicked, [&]{
            addMetrics(model, argv[1]);
    });
    win.connect(winui.callAddMetrics, &QPushButton::clicked, [&]{
            addMetrics(model, argv[1]);
    });

    // Frame and call graphs
    HistogramView* histFrame = new HistogramView(0);
    MetricGraphData dataFrame = MetricGraphData(&modelFrame, 1);
    histFrame->setDataProvider(&dataFrame);
    histFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    winui.graphsLayoutFrame->addWidget(histFrame);

    HistogramView* histCall = new HistogramView(0);
    MetricGraphData dataCall = MetricGraphData(&modelCall, 4);
    histCall->setDataProvider(&dataCall);
    histCall->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    winui.graphsLayoutCall->addWidget(histCall);

    // Selected column is plotted
    win.connect(winui.frameTableView->selectionModel(),
                 &QItemSelectionModel::selectionChanged, [&] {
        QList<QModelIndex> columns = winui.frameTableView->selectionModel()->selectedColumns();
        if (!columns.empty()) {
            dataFrame.setColumn(columns[0].column());
            histFrame->update();
        }
    });
    win.connect(winui.callTableView->selectionModel(),
                 &QItemSelectionModel::selectionChanged, [&] {
        QList<QModelIndex> columns = winui.callTableView->selectionModel()->selectedColumns();
        if (!columns.empty()) {
            dataCall.setColumn(columns[0].column());
            histCall->update();
        }
    });

    win.show();
    return app.exec();

}
