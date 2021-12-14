#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QUrl>
#include <QtCore>
//QFileSystemWatcher * watcher;

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    ~Worker();

signals:

public slots:
    void get(QString location);
    void post(QString location, QByteArray data);
    void strat_command(QString command, QString param, QString get_post, QString server);
    bool UpdateFileTimestamp(std::string fileName);
    void fileChangedEvent(const QString & path);
    QJsonDocument ReadJson(const QString &path);
    void run();

private slots:
    void readyRead();
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void encrypted(QNetworkReply *reply);
    void finished(QNetworkReply *reply);
    void preSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);


private:
    QNetworkAccessManager manager;
    QFileSystemWatcher * watcher;
};

#endif // WORKER_H
