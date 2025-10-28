#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include <QVariant>
#include <QString>

/**
 * @brief Singleton class for managing application settings
 *
 * This class provides centralized settings management to replace
 * duplicate loadsettings/savesettings implementations across
 * MainWindow, settingsDialog, and stocksDialog classes.
 */
class SettingsManager
{
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static SettingsManager& instance();

    /**
     * @brief Load a setting value
     * @param key The setting key
     * @param defaultValue Optional default value if key doesn't exist
     * @return The setting value as QVariant
     */
    QVariant loadSetting(const QString& key, const QVariant& defaultValue = QVariant());

    /**
     * @brief Save a setting value
     * @param key The setting key
     * @param value The value to save
     */
    void saveSetting(const QString& key, const QVariant& value);

    /**
     * @brief Set the application group name
     * @param group The group name (default: "coinbrowser")
     */
    void setAppGroup(const QString& group);

    /**
     * @brief Get the current application group name
     * @return The current group name
     */
    QString appGroup() const;

private:
    SettingsManager();  // Private constructor for singleton
    ~SettingsManager() = default;

    // Delete copy constructor and assignment operator
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QString m_appGroup;
    static const QString ORGANIZATION_NAME;
};

#endif // SETTINGSMANAGER_H
