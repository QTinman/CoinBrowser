#include "stocks.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>
#include <QGuiApplication>
#include <QtQml/QQmlFileSelector>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QtGlobal>

stocks::stocks(QObject *parent) : QObject(parent)
{
    //QGui app(argc, argv);
    if (qEnvironmentVariableIsEmpty("QML_XHR_ALLOW_FILE_READ"))
        qputenv("QML_XHR_ALLOW_FILE_READ", "1");
    QQuickView view;
    view.connect(view.engine(), &QQmlEngine::quit, this, &QCoreApplication::quit);
    view.setSource(QUrl("qrc:/demos/stocqt/stocqt.qml"));
    //if (view.status() == QQuickView::Error)
    //    ui->messages->setText("Error showing records.");
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();
}
