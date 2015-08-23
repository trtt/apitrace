#include <QApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QUrl>
#include <QFile>
#include <QDebug>

#include "metric_selection_model.hpp"
#include "ui_metric_selection.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFile file(":/test.txt");
    file.open(QIODevice::ReadOnly);
    MetricSelectionModel model(file);
    file.close();

    QDialog* widget = new QDialog;
    Ui::Dialog ui;
    ui.setupUi(widget);

    ui.treeView->setModel(&model);
    ui.treeView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (widget->exec()) {
        QString cliOption;
        QHash<QString, QList<MetricItem*>> mFrame;
        QHash<QString, QList<MetricItem*>> mCall;
        model.generateMetricList(cliOption, mFrame, mCall);
        qDebug() << cliOption;
        qDebug() << mFrame;
        qDebug() << mCall;
    };
    return app.exec();

}
