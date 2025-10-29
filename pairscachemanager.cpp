#include "pairscachemanager.h"
#include "constants.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QVariant>

PairsCacheManager::PairsCacheManager()
    : m_db(nullptr)
    , m_initialized(false)
{
}

PairsCacheManager& PairsCacheManager::instance()
{
    static PairsCacheManager instance;
    return instance;
}

bool PairsCacheManager::initialize(QSqlDatabase& db)
{
    m_db = &db;
    m_initialized = createCacheTable();
    return m_initialized;
}

bool PairsCacheManager::createCacheTable()
{
    if (!m_db || !m_db->isOpen()) {
        qDebug() << "Database not open for pairs cache";
        return false;
    }

    QSqlQuery query(*m_db);

    // Create pairs cache table with indexes for fast queries
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS pairs_cache (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            exchange TEXT NOT NULL,
            pair TEXT NOT NULL,
            base_currency TEXT NOT NULL,
            quote_currency TEXT NOT NULL,
            cached_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(exchange, pair)
        )
    )";

    if (!query.exec(createTable)) {
        qDebug() << "Failed to create pairs_cache table:" << query.lastError().text();
        return false;
    }

    // Create indexes for fast lookups
    query.exec("CREATE INDEX IF NOT EXISTS idx_pairs_exchange ON pairs_cache(exchange)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_pairs_quote ON pairs_cache(quote_currency)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_pairs_base ON pairs_cache(base_currency)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_pairs_timestamp ON pairs_cache(cached_at)");

    return true;
}

bool PairsCacheManager::isCacheFresh(const QString& exchange, int maxAgeHours)
{
    if (!m_initialized || !m_db) return false;

    QSqlQuery query(*m_db);
    query.prepare("SELECT COUNT(*), MAX(cached_at) FROM pairs_cache WHERE exchange = :exchange");
    query.bindValue(":exchange", exchange.toLower());

    if (!query.exec() || !query.next()) {
        return false;
    }

    int count = query.value(0).toInt();
    if (count == 0) return false;

    QDateTime cachedAt = query.value(1).toDateTime();
    QDateTime now = QDateTime::currentDateTime();

    qint64 hoursDiff = cachedAt.secsTo(now) / 3600;

    bool isFresh = hoursDiff < maxAgeHours;
    qDebug() << "Cache for" << exchange << "is" << hoursDiff << "hours old. Fresh:" << isFresh;

    return isFresh;
}

QStringList PairsCacheManager::getPairs(const QString& exchange,
                                        const QString& quoteCurrency,
                                        const QSet<QString>& blacklist,
                                        const QString& textFilePath)
{
    if (!m_initialized) {
        qDebug() << "PairsCacheManager not initialized, falling back to text file";
        return parseTextFile(textFilePath);
    }

    // Check if cache is fresh
    if (isCacheFresh(exchange)) {
        qDebug() << "Using cached pairs for" << exchange;
        QStringList cached = loadFromCache(exchange, quoteCurrency, blacklist);
        if (!cached.isEmpty()) {
            return cached;
        }
    }

    // Cache is stale or empty, refresh from text file
    qDebug() << "Cache stale for" << exchange << ", refreshing from file:" << textFilePath;
    int count = refreshCacheFromFile(exchange, textFilePath);
    qDebug() << "Cached" << count << "pairs for" << exchange;

    // Now load from cache with filtering
    return loadFromCache(exchange, quoteCurrency, blacklist);
}

QStringList PairsCacheManager::parseTextFile(const QString& filePath)
{
    QStringList allPairs;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file:" << filePath;
        return allPairs;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();

        // Parse format: "    BTC/USDT    "
        // Find the "/" separator
        int middle = line.indexOf("/", 10);
        if (middle == -1) continue;

        int start = line.lastIndexOf(" ", middle);
        int end = line.indexOf(" ", middle);

        if (start == -1 || end == -1) continue;

        QString pair = line.mid(start + 1, end - start - 1).trimmed();
        if (!pair.isEmpty() && pair.contains("/")) {
            allPairs.append(pair);
        }
    }

    file.close();
    return allPairs;
}

int PairsCacheManager::refreshCacheFromFile(const QString& exchange, const QString& textFilePath)
{
    QStringList pairs = parseTextFile(textFilePath);
    if (pairs.isEmpty()) {
        qDebug() << "No pairs found in file:" << textFilePath;
        return 0;
    }

    // Clear old cache for this exchange
    clearCache(exchange);

    // Insert new pairs
    if (!cachePairs(exchange, pairs)) {
        return 0;
    }

    return pairs.size();
}

bool PairsCacheManager::cachePairs(const QString& exchange, const QStringList& pairs)
{
    if (!m_db || !m_db->isOpen()) return false;

    QSqlQuery query(*m_db);

    // Begin transaction for faster bulk insert
    m_db->transaction();

    query.prepare(R"(
        INSERT OR REPLACE INTO pairs_cache
        (exchange, pair, base_currency, quote_currency, cached_at)
        VALUES (:exchange, :pair, :base, :quote, CURRENT_TIMESTAMP)
    )");

    int inserted = 0;
    for (const QString& pair : pairs) {
        QStringList parts = pair.split("/");
        if (parts.size() != 2) continue;

        QString base = parts[0].trimmed();
        QString quote = parts[1].trimmed();

        query.bindValue(":exchange", exchange.toLower());
        query.bindValue(":pair", pair);
        query.bindValue(":base", base);
        query.bindValue(":quote", quote);

        if (query.exec()) {
            inserted++;
        } else {
            qDebug() << "Failed to insert pair:" << pair << query.lastError().text();
        }
    }

    m_db->commit();
    qDebug() << "Cached" << inserted << "pairs for" << exchange;

    return inserted > 0;
}

QStringList PairsCacheManager::loadFromCache(const QString& exchange,
                                             const QString& quoteCurrency,
                                             const QSet<QString>& blacklist)
{
    QStringList result;
    if (!m_db || !m_db->isOpen()) return result;

    QSqlQuery query(*m_db);

    // Use indexed query for fast filtering
    query.prepare(R"(
        SELECT base_currency
        FROM pairs_cache
        WHERE exchange = :exchange
        AND quote_currency = :quote
        ORDER BY base_currency
    )");

    query.bindValue(":exchange", exchange.toLower());
    query.bindValue(":quote", quoteCurrency);

    if (!query.exec()) {
        qDebug() << "Failed to load from cache:" << query.lastError().text();
        return result;
    }

    // Apply blacklist filtering
    while (query.next()) {
        QString base = query.value(0).toString();
        if (!blacklist.contains(base)) {
            result.append(base);
        }
    }

    return result;
}

void PairsCacheManager::clearCache(const QString& exchange)
{
    if (!m_db || !m_db->isOpen()) return;

    QSqlQuery query(*m_db);

    if (exchange.isEmpty()) {
        query.exec("DELETE FROM pairs_cache");
        qDebug() << "Cleared all pairs cache";
    } else {
        query.prepare("DELETE FROM pairs_cache WHERE exchange = :exchange");
        query.bindValue(":exchange", exchange.toLower());
        query.exec();
        qDebug() << "Cleared cache for" << exchange;
    }
}

QString PairsCacheManager::getCacheStats(const QString& exchange)
{
    if (!m_db || !m_db->isOpen()) return "Cache not initialized";

    QSqlQuery query(*m_db);

    if (exchange.isEmpty()) {
        query.exec("SELECT COUNT(*), COUNT(DISTINCT exchange) FROM pairs_cache");
    } else {
        query.prepare("SELECT COUNT(*), MAX(cached_at) FROM pairs_cache WHERE exchange = :exchange");
        query.bindValue(":exchange", exchange.toLower());
        query.exec();
    }

    if (query.next()) {
        int count = query.value(0).toInt();
        QString timestamp = query.value(1).toString();
        return QString("Pairs: %1, Last cached: %2").arg(count).arg(timestamp);
    }

    return "No cache data";
}
