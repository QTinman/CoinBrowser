#ifndef JSONFILEMANAGER_H
#define JSONFILEMANAGER_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>

/**
 * @brief Utility class for reading and parsing JSON files
 *
 * This class provides centralized JSON file management to replace
 * duplicate ReadJson implementations across MainWindow, Worker,
 * and stocksDialog classes.
 */
class JsonFileManager
{
public:
    /**
     * @brief Read and parse a JSON file
     * @param path The file path to read
     * @param dataKey The key to extract from the JSON object (default: "data")
     * @return QJsonArray containing the parsed data
     */
    static QJsonArray readJsonArray(const QString& path, const QString& dataKey = "data");

    /**
     * @brief Read and parse a JSON file into an object
     * @param path The file path to read
     * @return QJsonObject containing the parsed data
     */
    static QJsonObject readJsonObject(const QString& path);

    /**
     * @brief Check if a JSON file exists and is readable
     * @param path The file path to check
     * @return true if file exists and is readable, false otherwise
     */
    static bool isValidJsonFile(const QString& path);

private:
    JsonFileManager() = default;  // Utility class, no instances needed
};

#endif // JSONFILEMANAGER_H
