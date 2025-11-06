#ifndef EXCHANGEAPI_H
#define EXCHANGEAPI_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>

/**
 * @brief Base class for exchange API adapters
 *
 * Provides a unified interface for fetching cryptocurrency market data
 * from various exchanges (Binance, Kraken, Coinbase, etc.) to replace
 * the CoinMarketCap API.
 *
 * Benefits:
 * - No API key required for most exchanges
 * - More accurate real-time data
 * - No rate limits (or much higher limits)
 * - Direct from source (exchanges)
 */
class ExchangeAPI : public QObject
{
    Q_OBJECT

public:
    explicit ExchangeAPI(QObject *parent = nullptr);
    virtual ~ExchangeAPI() = default;

    /**
     * @brief Get the exchange name
     */
    virtual QString exchangeName() const = 0;

    /**
     * @brief Fetch all ticker data for a quote currency
     * @param quoteCurrency Quote currency (e.g., "USDT", "BTC", "USD")
     * @param pairs Optional list of specific pairs to fetch
     */
    virtual void fetchTickers(const QString& quoteCurrency, const QStringList& pairs = QStringList()) = 0;

    /**
     * @brief Get the API endpoint URL
     */
    virtual QString apiEndpoint() const = 0;

    /**
     * @brief Check if this exchange requires an API key
     */
    virtual bool requiresApiKey() const { return false; }

    /**
     * @brief Set API key (if required)
     */
    virtual void setApiKey(const QString& key) { Q_UNUSED(key); }

signals:
    /**
     * @brief Emitted when ticker data is successfully fetched
     * @param data JSON array in CoinMarketCap-compatible format
     */
    void tickersReceived(const QJsonArray& data);

    /**
     * @brief Emitted when an error occurs
     */
    void errorOccurred(const QString& error);

    /**
     * @brief Progress update
     */
    void progressUpdate(int current, int total);

protected:
    /**
     * @brief Transform exchange-specific JSON to CMC-compatible format
     * @param exchangeData Raw JSON from exchange API
     * @param quoteCurrency Quote currency for the data
     * @return CoinMarketCap-compatible JSON array
     */
    virtual QJsonArray transformToCMCFormat(const QJsonDocument& exchangeData, const QString& quoteCurrency) = 0;

    /**
     * @brief Make HTTP GET request
     */
    void makeRequest(const QUrl& url, QNetworkAccessManager* manager);

    QNetworkAccessManager* m_networkManager;
};

/**
 * @brief Binance exchange API adapter
 *
 * Uses Binance public API:
 * - GET /api/v3/ticker/24hr - All 24h tickers
 * - No API key required
 * - Very fast, real-time data
 */
class BinanceAPI : public ExchangeAPI
{
    Q_OBJECT

public:
    explicit BinanceAPI(QObject *parent = nullptr);

    QString exchangeName() const override { return "Binance"; }
    QString apiEndpoint() const override { return "https://api.binance.com"; }
    void fetchTickers(const QString& quoteCurrency, const QStringList& pairs = QStringList()) override;

protected:
    QJsonArray transformToCMCFormat(const QJsonDocument& exchangeData, const QString& quoteCurrency) override;

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_quoteCurrency;
    QStringList m_requestedPairs;
};

/**
 * @brief Kraken exchange API adapter
 *
 * Uses Kraken public API:
 * - GET /0/public/Ticker - Ticker information
 * - No API key required
 */
class KrakenAPI : public ExchangeAPI
{
    Q_OBJECT

public:
    explicit KrakenAPI(QObject *parent = nullptr);

    QString exchangeName() const override { return "Kraken"; }
    QString apiEndpoint() const override { return "https://api.kraken.com"; }
    void fetchTickers(const QString& quoteCurrency, const QStringList& pairs = QStringList()) override;

protected:
    QJsonArray transformToCMCFormat(const QJsonDocument& exchangeData, const QString& quoteCurrency) override;

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_quoteCurrency;
};

/**
 * @brief Factory for creating exchange API instances
 */
class ExchangeAPIFactory
{
public:
    /**
     * @brief Create an API instance for a specific exchange
     * @param exchangeName Exchange name (case-insensitive)
     * @param parent Parent QObject
     * @return ExchangeAPI instance or nullptr if exchange not supported
     */
    static ExchangeAPI* createAPI(const QString& exchangeName, QObject* parent = nullptr);

    /**
     * @brief Get list of supported exchanges
     */
    static QStringList supportedExchanges();

    /**
     * @brief Check if an exchange is supported
     */
    static bool isSupported(const QString& exchangeName);
};

#endif // EXCHANGEAPI_H
