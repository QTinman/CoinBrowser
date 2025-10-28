#include "settingsmanager.h"

const QString SettingsManager::ORGANIZATION_NAME = "QTinman";

SettingsManager::SettingsManager()
    : m_appGroup("coinbrowser")
{
}

SettingsManager& SettingsManager::instance()
{
    static SettingsManager instance;
    return instance;
}

QVariant SettingsManager::loadSetting(const QString& key, const QVariant& defaultValue)
{
    QSettings settings(ORGANIZATION_NAME, m_appGroup);
    settings.beginGroup(m_appGroup);
    QVariant value = settings.value(key, defaultValue);
    settings.endGroup();
    return value;
}

void SettingsManager::saveSetting(const QString& key, const QVariant& value)
{
    QSettings settings(ORGANIZATION_NAME, m_appGroup);
    settings.beginGroup(m_appGroup);
    settings.setValue(key, QVariant::fromValue(value));
    settings.endGroup();
}

void SettingsManager::setAppGroup(const QString& group)
{
    m_appGroup = group;
}

QString SettingsManager::appGroup() const
{
    return m_appGroup;
}
