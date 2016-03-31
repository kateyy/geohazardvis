#include "ApplicationSettings.h"

#include <QFile>
#include <QStandardPaths>

namespace
{

const QString & settingsFileName()
{
    static const QString fn = 
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini";

    return fn;
}
}


ApplicationSettings::ApplicationSettings(QObject * parent)
    : QSettings(settingsFileName(), QSettings::IniFormat, parent)
{
}

ApplicationSettings::~ApplicationSettings() = default;

bool ApplicationSettings::settingsExist()
{
    return QFile::exists(settingsFileName());
}
