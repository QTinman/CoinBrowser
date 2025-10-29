#ifndef PAIRSCACHEMANAGER_H
#define PAIRSCACHEMANAGER_H

#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSet>

/**
 * @brief High-performance pairs caching system using SQLite
 *
 * This class replaces slow text file parsing with fast SQLite queries,
 * dramatically reducing startup time from seconds to milliseconds.
 *
 * Features:
 * - Caches trading pairs in indexed SQLite table
 * - Automatic cache expiration (default: 24 hours)
 * - O(1) blacklist filtering with indexed queries
 * - Falls back to text files when cache is stale
 * - Future: Direct exchange API integration
 */
class PairsCacheManager
{
public:
    /**
     * @brief Get the singleton instance
     */
    static PairsCacheManager& instance();

    /**
     * @brief Get pairs for an exchange (from cache or text file)
     * @param exchange Exchange name (e.g., "Binance")
     * @param quoteCurrency Quote currency (e.g., "USDT", "BTC")
     * @param blacklist Set of blacklisted base currencies
     * @param textFilePath Path to fallback text file
     * @return List of trading pairs
     */
    QStringList getPairs(const QString& exchange,
                        const QString& quoteCurrency,
                        const QSet<QString>& blacklist,
                        const QString& textFilePath);

    /**
     * @brief Check if cache is fresh for an exchange
     * @param exchange Exchange name
     * @param maxAgeHours Maximum cache age in hours (default: 24)
     * @return true if cache is fresh, false otherwise
     */
    bool isCacheFresh(const QString& exchange, int maxAgeHours = 24);

    /**
     * @brief Force refresh cache from text file
     * @param exchange Exchange name
     * @param textFilePath Path to text file
     * @return Number of pairs cached
     */
    int refreshCacheFromFile(const QString& exchange, const QString& textFilePath);

    /**
     * @brief Clear all cached pairs for an exchange
     * @param exchange Exchange name (empty = clear all)
     */
    void clearCache(const QString& exchange = "");

    /**
     * @brief Initialize the cache database
     * @param db Database connection
     * @return true if successful
     */
    bool initialize(QSqlDatabase& db);

    /**
     * @brief Get cache statistics
     * @return QString with cache info (for debugging)
     */
    QString getCacheStats(const QString& exchange);

private:
    PairsCacheManager();
    ~PairsCacheManager() = default;

    // Delete copy constructor and assignment
    PairsCacheManager(const PairsCacheManager&) = delete;
    PairsCacheManager& operator=(const PairsCacheManager&) = delete;

    /**
     * @brief Create the pairs cache table if it doesn't exist
     */
    bool createCacheTable();

    /**
     * @brief Parse text file and extract pairs
     */
    QStringList parseTextFile(const QString& filePath);

    /**
     * @brief Store pairs in cache
     */
    bool cachePairs(const QString& exchange, const QStringList& pairs);

    /**
     * @brief Load pairs from cache with blacklist filtering
     */
    QStringList loadFromCache(const QString& exchange,
                              const QString& quoteCurrency,
                              const QSet<QString>& blacklist);

    QSqlDatabase* m_db;
    bool m_initialized;
};

#endif // PAIRSCACHEMANAGER_H
