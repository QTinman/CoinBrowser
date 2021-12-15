#include "worker.h"
#include <QtCore>


#include "worker.h"
#include <ctime>
#include <sys/stat.h>
#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

//QString fname="candledata.json";




bool Worker::UpdateFileTimestamp(std::string fileName) {
    struct stat fstat;
    struct utimbuf new_time;

    if (0 != stat(fileName.c_str(), &fstat))
        return false;

    new_time.actime = fstat.st_atime;
    new_time.modtime = time(NULL);
    if (0 != utime(fileName.c_str(), &new_time))
        return false;

    return true;
}

void Worker::fileChangedEvent(const QString & path)
{
  //qDebug() << path;
  //QJsonDocument jdoc=ReadJson(filename);
  QFile file;
  //file.setFileName(filename);
  //qDebug() << this->objectName();

  //if (!jdoc.isEmpty()) qDebug() << jdoc;
  //else qDebug() << "No data!";
}

QJsonDocument Worker::ReadJson(const QString &path)
{
    QFile file( path );
    QJsonArray jsonArray;
    QJsonDocument cryptolist;
    if( file.open( QIODevice::ReadOnly ) )
    {
        QByteArray bytes = file.readAll();
        file.close();

        QJsonParseError parserError;
        cryptolist = QJsonDocument::fromJson(bytes, &parserError);
        QJsonObject jsonObject = cryptolist.object();

    }
    return cryptolist;
}

void Worker::run()
{
    //qInfo() << QThread::currentThread();
    QString pair=QThread::currentThread()->objectName();
    //qInfo() << pair;
    //filename="./files/"+pair+".json";
    //do_download("binance","1h",pair,33,1639139047701);
    this->deleteLater();
}

Worker::Worker(QObject *parent)
{
    Q_UNUSED(parent);
    connect(&manager,&QNetworkAccessManager::authenticationRequired,this,&Worker::authenticationRequired);
    connect(&manager,&QNetworkAccessManager::encrypted,this,&Worker::encrypted);
    connect(&manager,&QNetworkAccessManager::preSharedKeyAuthenticationRequired,this,&Worker::preSharedKeyAuthenticationRequired);
    connect(&manager,&QNetworkAccessManager::finished,this,&Worker::finished);
    connect(&manager,&QNetworkAccessManager::authenticationRequired,this,&Worker::authenticationRequired);
    connect(&manager,&QNetworkAccessManager::sslErrors,this,&Worker::sslErrors);
    watcher = new QFileSystemWatcher();
    QString pair=QThread::currentThread()->objectName();
    //watcher->addPath(filename);
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChangedEvent(QString)));

    //qInfo() << this << " Constructed " << QThread::currentThread();
}

Worker::~Worker()
{

}

void Worker::get(QString location)
{
    //qInfo() << "Getting from server...";
    QNetworkRequest request = QNetworkRequest(QUrl(location));
    QNetworkReply *reply=manager.get(QNetworkRequest(request));
    connect(reply,&QNetworkReply::readyRead,this,&Worker::readyRead);
}

void Worker::post(QString location, QByteArray data)
{
    //qInfo() << "Posting to server...";
    QNetworkRequest request = QNetworkRequest(QUrl(location));
    QNetworkReply *reply=manager.post(request,data);
    connect(reply,&QNetworkReply::readyRead,this,&Worker::readyRead);
}

void Worker::readyRead()
{
    //qInfo() << "readyRead";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if(reply->error())
    {
        QByteArray rawtable=reply->readAll();
        qDebug() << "Error: " << reply->error() <<
        ", Message: " << reply->errorString() <<
        ", Code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
        ", Description: " << rawtable;
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 429)
            get(reply->url().toString());
    }
    else
    {

        //qInfo() << reply->readBufferSize();
        QByteArray data=reply->readAll();
        //if (!data.isEmpty()) qInfo() << data;
        QString pair=reply->url().toString();
        int start,end;
        start=pair.indexOf("symbol=");
        end=pair.indexOf("&",start);
        pair=pair.mid(start+7,end-start-7);

        QFile *file = new QFile("./files/"+pair+".json");
        if (file->error()) qDebug() << file->errorString();
        if(file->open(QFile::WriteOnly))
        {
            file->write(data);
            file->flush();
            file->close();
        }
        delete file;
    }
}

void Worker::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    Q_UNUSED(authenticator);
    //qInfo() << "authenticationRequired";
}

void Worker::encrypted(QNetworkReply *reply)
{
    Q_UNUSED(reply);
    //qInfo() << "encrypted";
}

void Worker::finished(QNetworkReply *reply)
{
    //Q_UNUSED(reply);

    //qInfo() << "finished";
    if(reply->error())
    {
        QByteArray rawtable=reply->readAll();
        qDebug() << "Error: " << reply->error() <<
        ". Url: " << reply->url() <<
        ", Message: " << reply->errorString() <<
        ", Code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
        ", Description: " << rawtable;
    }
}


void Worker::preSharedKeyAuthenticationRequired(QNetworkReply *reply, QSslPreSharedKeyAuthenticator *authenticator)
{
    Q_UNUSED(reply);
    Q_UNUSED(authenticator);
    //qInfo() << "preSharedKeyAuthenticationRequired";
}

void Worker::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_UNUSED(proxy);
    Q_UNUSED(authenticator);
    //qInfo() << "proxyAuthenticationRequired";
}

void Worker::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_UNUSED(reply);
    Q_UNUSED(errors);
    //qInfo() << "sslErrors";
}
