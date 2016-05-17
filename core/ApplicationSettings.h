#pragma once

#include <QSettings>

#include <core/core_api.h>


/**
QSettings subclass that accesses application settings from a default location.
*/
class CORE_API ApplicationSettings : public QSettings
{
public:
    explicit ApplicationSettings(QObject * parent = nullptr);
    ~ApplicationSettings() override;

    /** @return true, if there are any stored settings for the application.
      * Normally, this returns false only if the application never ran on the current system/user. */
    static bool settingsExist();
};
