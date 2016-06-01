#include "RuntimeInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>

#include "config.h"


const QString & RuntimeInfo::basePath()
{
    static const QString path = [] ()
    {
        const auto appPath = QCoreApplication::applicationDirPath();
        const auto installBinRelPath = QString(config::installBinRelativePath);
        const QString checkDirName = "data";

        QString _basePath;

        // mainly Windows: executable located directly in base folder
        if (installBinRelPath == ".")
        {
            if (QDir(appPath).exists(checkDirName))
            {
                _basePath = appPath;
            }
        }
        // mainly Linux: executable placed in something like "bin" subfolder
        else
        {
            if (appPath.length() >= installBinRelPath.length()
                && appPath.rightRef(installBinRelPath.length()) == installBinRelPath)
            {
                // /some/path/to/base_dir/bin/dir [/exename]
                // /some/path_to/base_dir <-> / <-> bin/dir
                _basePath = appPath.left(appPath.length() - (installBinRelPath.length() + 1));
            }
        }

        // dev: folder structure not setup yet
        if (_basePath.isEmpty())
        {
            _basePath = QDir::currentPath();
            qDebug() << "RuntimeInfo: Assuming to be running in out of source build. Using current working directory as base path.";
        }

        return _basePath;
    }();

    return path;
}

const QString & RuntimeInfo::dataPath()
{
#if !WIN32 && !defined(OPTION_LOCAL_INSTALL)
#error Non-local installation is not implemented on the current platform.
#endif

    static const QString path = QDir(basePath()).filePath(config::installDataRelativePath);

    return path;
}

const QString & RuntimeInfo::pluginsPath()
{
    static const QString path = QDir(basePath()).filePath(config::installPluginsSharedRelativePath);

    return path;
}
