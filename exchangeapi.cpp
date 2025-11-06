#include "exchangeapi.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <QUrlQuery>

// ========== ExchangeAPI Base Class ==========

ExchangeAPI::ExchangeAPI(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

void ExchangeAPI::makeRequest(const QUrl& url, QNetworkAccessManager* manager)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "CoinBrowser/1.0");
    manager->get(request);
}

// ========== BinanceAPI Implementation ==========

BinanceAPI::BinanceAPI(QObject *parent)
    : ExchangeAPI(parent)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &BinanceAPI::onReplyFinished);
}

void BinanceAPI::fetchTickers(const QString& quoteCurrency, const QStringList& pairs)
{
    m_quoteCurrency = quoteCurrency;
    m_requestedPairs = pairs;

    // Binance API: GET /api/v3/ticker/24hr
    // Returns all 24h ticker data
    QString endpoint = apiEndpoint() + "/api/v3/ticker/24hr";

    // If specific symbols requested, add them as parameter
    if (!pairs.isEmpty()) {
        QStringList symbols;
        for (const QString& pair : pairs) {
            symbols << "\"" + pair + quoteCurrency + "\"";
        }
        endpoint += "?symbols=[" + symbols.join(",") + "]";
    }

    qDebug() << "Fetching Binance tickers from:" << endpoint;
    makeRequest(QUrl(endpoint), m_networkManager);
}

void BinanceAPI::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        QString error = QString("Binance API error: %1").arg(reply->errorString());
        qDebug() << error;
        emit errorOccurred(error);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull() || (!doc.isArray() && !doc.isObject())) {
        emit errorOccurred("Invalid JSON response from Binance");
        reply->deleteLater();
        return;
    }

    // Transform to CMC format
    QJsonArray cmcFormat = transformToCMCFormat(doc, m_quoteCurrency);

    qDebug() << "Binance API: Transformed" << cmcFormat.size() << "tickers";
    emit tickersReceived(cmcFormat);

    reply->deleteLater();
}

QJsonArray BinanceAPI::transformToCMCFormat(const QJsonDocument& exchangeData, const QString& quoteCurrency)
{
    QJsonArray result;
    QJsonArray tickers = exchangeData.isArray() ? exchangeData.array() : exchangeData.object()["data"].toArray();

    int rank = 1;
    for (const QJsonValue& value : tickers) {
        QJsonObject ticker = value.toObject();
        QString symbol = ticker["symbol"].toString();

        // Filter by quote currency
        if (!symbol.endsWith(quoteCurrency)) {
            continue;
        }

        // Extract base currency (remove quote currency from end)
        QString baseCurrency = symbol;
        baseCurrency.chop(quoteCurrency.length());

        // Skip if not in requested pairs (if specified)
        if (!m_requestedPairs.isEmpty() && !m_requestedPairs.contains(baseCurrency)) {
            continue;
        }

        // Build CMC-compatible object
        QJsonObject coin;
        coin["cmc_rank"] = rank++;
        coin["symbol"] = baseCurrency;
        coin["name"] = baseCurrency; // Binance doesn't provide full name

        // Build quote object
        QJsonObject quoteData;
        quoteData["price"] = ticker["lastPrice"].toString().toDouble();
        quoteData["volume_24h"] = ticker["quoteVolume"].toString().toDouble();
        quoteData["percent_change_1h"] = 0.0; // Not available from Binance 24hr ticker
        quoteData["percent_change_24h"] = ticker["priceChangePercent"].toString().toDouble();
        quoteData["percent_change_7d"] = 0.0; // Not available
        quoteData["percent_change_30d"] = 0.0; // Not available
        quoteData["percent_change_60d"] = 0.0; // Not available
        quoteData["percent_change_90d"] = 0.0; // Not available
        quoteData["market_cap"] = 0.0; // Not available from ticker endpoint
        quoteData["last_updated"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonObject quote;
        quote[quoteCurrency] = quoteData;
        coin["quote"] = quote;

        result.append(coin);
    }

    return result;
}

// ========== KrakenAPI Implementation ==========

KrakenAPI::KrakenAPI(QObject *parent)
    : ExchangeAPI(parent)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &KrakenAPI::onReplyFinished);
}

void KrakenAPI::fetchTickers(const QString& quoteCurrency, const QStringList& pairs)
{
    m_quoteCurrency = quoteCurrency;

    // Kraken uses different quote currency names
    QString krakenQuote = quoteCurrency;
    if (quoteCurrency == "USDT") krakenQuote = "USDT";
    else if (quoteCurrency == "BTC") krakenQuote = "XBT"; // Kraken uses XBT for BTC
    else if (quoteCurrency == "USD") krakenQuote = "USD";

    // Build pair list for Kraken (format: BTC/USD -> XXBTZUSD)
    QString endpoint = apiEndpoint() + "/0/public/Ticker";

    qDebug() << "Fetching Kraken tickers from:" << endpoint;
    makeRequest(QUrl(endpoint), m_networkManager);
}

void KrakenAPI::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        QString error = QString("Kraken API error: %1").arg(reply->errorString());
        qDebug() << error;
        emit errorOccurred(error);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    if (doc.isNull()) {
        emit errorOccurred("Invalid JSON response from Kraken");
        reply->deleteLater();
        return;
    }

    QJsonArray cmcFormat = transformToCMCFormat(doc, m_quoteCurrency);

    qDebug() << "Kraken API: Transformed" << cmcFormat.size() << "tickers";
    emit tickersReceived(cmcFormat);

    reply->deleteLater();
}

QJsonArray KrakenAPI::transformToCMCFormat(const QJsonDocument& exchangeData, const QString& quoteCurrency)
{
    QJsonArray result;
    QJsonObject root = exchangeData.object();
    QJsonObject resultObj = root["result"].toObject();

    int rank = 1;
    for (const QString& pairKey : resultObj.keys()) {
        QJsonObject ticker = resultObj[pairKey].toObject();

        // Try to parse pair name (Kraken format is complex)
        QString baseCurrency = pairKey;
        // Simple heuristic: remove common quote currencies from end
        if (baseCurrency.endsWith("USDT")) baseCurrency.chop(4);
        else if (baseCurrency.endsWith("USD")) baseCurrency.chop(3);
        else if (baseCurrency.endsWith("EUR")) baseCurrency.chop(3);
        else if (baseCurrency.endsWith("XBT")) baseCurrency.chop(3);
        else continue; // Skip if we can't parse

        // Remove Kraken prefix (X or Z)
        if (baseCurrency.startsWith("X") || baseCurrency.startsWith("Z")) {
            baseCurrency = baseCurrency.mid(1);
        }

        QJsonObject coin;
        coin["cmc_rank"] = rank++;
        coin["symbol"] = baseCurrency;
        coin["name"] = baseCurrency;

        // Kraken ticker structure: [last, volume, ...]
        QJsonArray lastPrice = ticker["c"].toArray();
        QJsonArray volume = ticker["v"].toArray();

        QJsonObject quoteData;
        quoteData["price"] = lastPrice.size() > 0 ? lastPrice[0].toString().toDouble() : 0.0;
        quoteData["volume_24h"] = volume.size() > 1 ? volume[1].toString().toDouble() : 0.0;
        quoteData["percent_change_1h"] = 0.0;
        quoteData["percent_change_24h"] = 0.0; // Would need to calculate from open price
        quoteData["percent_change_7d"] = 0.0;
        quoteData["percent_change_30d"] = 0.0;
        quoteData["percent_change_60d"] = 0.0;
        quoteData["percent_change_90d"] = 0.0;
        quoteData["market_cap"] = 0.0;
        quoteData["last_updated"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonObject quote;
        quote[quoteCurrency] = quoteData;
        coin["quote"] = quote;

        result.append(coin);
    }

    return result;
}

// ========== ExchangeAPIFactory Implementation ==========

ExchangeAPI* ExchangeAPIFactory::createAPI(const QString& exchangeName, QObject* parent)
{
    QString name = exchangeName.toLower();

    if (name == "binance") {
        return new BinanceAPI(parent);
    } else if (name == "kraken") {
        return new KrakenAPI(parent);
    }

    qDebug() << "Unsupported exchange:" << exchangeName;
    return nullptr;
}

QStringList ExchangeAPIFactory::supportedExchanges()
{
    return {"Binance", "Kraken"};
}

bool ExchangeAPIFactory::isSupported(const QString& exchangeName)
{
    return supportedExchanges().contains(exchangeName, Qt::CaseInsensitive);
}
