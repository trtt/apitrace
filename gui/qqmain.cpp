#include <QApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QUrl>
#include <QFile>
#include <QDebug>
#include <QProcess>

#include "metric_selection_model.hpp"
#include "ui_metric_selection.h"

#include <unistd.h>

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
    //BlockingIODevice io (&process);
    //QFile file(":/test.txt");
    //file.open(QIODevice::ReadOnly);
    //qDebug() << process.readAllStandardOutput();
    MetricSelectionModel model(process);
    //file.close();

    QDialog* widget = new QDialog;
    Ui::Dialog ui;
    ui.setupUi(widget);

    ui.treeView->setModel(&model);
    ui.treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (widget->exec()) {
        QString cliOptionFrame, cliOptionCall;
        QHash<QString, QList<MetricItem*>> mFrame;
        QHash<QString, QList<MetricItem*>> mCall;
        model.generateMetricList(cliOptionFrame, cliOptionCall, mFrame, mCall);
        //qDebug() << cliOption;
        //qDebug() << mFrame;
        //qDebug() << mCall;
        //qDebug() << cliOption.toStdString().c_str();
        execl("./glretrace", "./glretrace", cliOptionFrame.toStdString().c_str(),
              cliOptionCall.toStdString().c_str(), argv[1], NULL);
    };
    return app.exec();

}
