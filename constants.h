#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * @file constants.h
 * @brief Application-wide constants for CoinBrowser
 *
 * This file centralizes all magic numbers used throughout the application
 * to improve maintainability and code clarity.
 */

namespace Constants {

// Time constants (in seconds)
constexpr int DEFAULT_UPDATE_INTERVAL_MINUTES = 60;
constexpr int DEFAULT_AUTO_UPDATE_SECONDS = 1800;  // 30 minutes
constexpr int ONE_HOUR_SECONDS = 3600;

// Time constants (in milliseconds)
constexpr int DELAY_SHORT_MS = 20;
constexpr int DELAY_MEDIUM_MS = 100;
constexpr int DELAY_LONG_MS = 150;
constexpr int DELAY_ONE_SECOND_MS = 1000;
constexpr int DELAY_FIVE_SECONDS_MS = 5000;

// Database constants
constexpr int TABLE_MAX_AGE_MINUTES = 999;
constexpr int DEFAULT_TABLE_COLUMNS = 11;

// API and data limits
constexpr int API_COIN_LIMIT = 5000;
constexpr int DEFAULT_COIN_FROM = 1;
constexpr int DEFAULT_COIN_TO = 1;

// Price filter defaults
constexpr double DEFAULT_CHANGE_1H_FROM = -2.0;
constexpr double DEFAULT_CHANGE_1H_TO = 5.0;
constexpr double DEFAULT_CHANGE_24H_FROM = 0.0;
constexpr double DEFAULT_CHANGE_24H_TO = 100.0;
constexpr double DEFAULT_CHANGE_7D_FROM = -2.0;
constexpr double DEFAULT_CHANGE_7D_TO = 100.0;

// Network and timing
constexpr int PROCESS_EVENTS_TIMEOUT_MS = 100;
constexpr int NETWORK_TIMEOUT_MS = 1000;

// Default application values
constexpr double DEFAULT_BTC_PRICE = 58338.0;

// Conversion factors
constexpr int MILLISECONDS_PER_MINUTE = 60000;
constexpr int MILLION = 1000000;

// Application defaults
const char APP_GROUP[] = "coinbrowser";
const char DB_FILE[] = "coinhistory.db";

} // namespace Constants

#endif // CONSTANTS_H
