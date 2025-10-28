#include "jsonfilemanager.h"
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

QJsonArray JsonFileManager::readJsonArray(const QString& path, const QString& dataKey)
{
    QJsonArray jsonArray;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error: Cannot open file" << path;
        return jsonArray;
    }

    QByteArray bytes = file.readAll();
    file.close();

    QJsonParseError parserError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &parserError);

    if (parserError.error != QJsonParseError::NoError) {
        qDebug() << "JSON Parse Error:" << parserError.errorString();
        return jsonArray;
    }

    // Try to extract array from object if dataKey is provided
    if (jsonDoc.isObject() && !dataKey.isEmpty()) {
        QJsonObject jsonObject = jsonDoc.object();
        if (jsonObject.contains(dataKey)) {
            jsonArray = jsonObject[dataKey].toArray();
        }
    } else if (jsonDoc.isArray()) {
        jsonArray = jsonDoc.array();
    }

    return jsonArray;
}

QJsonObject JsonFileManager::readJsonObject(const QString& path)
{
    QJsonObject jsonObject;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Error: Cannot open file" << path;
        return jsonObject;
    }

    QByteArray bytes = file.readAll();
    file.close();

    QJsonParseError parserError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &parserError);

    if (parserError.error != QJsonParseError::NoError) {
        qDebug() << "JSON Parse Error:" << parserError.errorString();
        return jsonObject;
    }

    if (jsonDoc.isObject()) {
        jsonObject = jsonDoc.object();
    }

    return jsonObject;
}

bool JsonFileManager::isValidJsonFile(const QString& path)
{
    QFile file(path);
    return file.exists() && file.open(QIODevice::ReadOnly);
}
