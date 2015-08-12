#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QMainWindow>
#include <QTableView>

#include "metric_selection_model.hpp"
#include "metric_data_model.hpp"

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

    win.show();
    return app.exec();

}
