#include "RuntimeInfo.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>

#include "config.h"


namespace
{

struct RuntimeInfo_internal
{
    bool isInstalled;
    QString binariesBaseDir;
    QString dataBaseDir;

    static const RuntimeInfo_internal & instance()
    {
        static const RuntimeInfo_internal _instance;
        return _instance;
    }

private:
    RuntimeInfo_internal()
    {
        const auto appPath = QCoreApplication::applicationDirPath();
        const auto workingDir = QDir::currentPath();
        const auto installBinRelPath = QString(config::installBinRelativePath);
        const QString checkDirName = "data";

        // mainly Windows: executable located directly in base folder
        if (installBinRelPath == ".")
        {
            if (QDir(appPath).exists(checkDirName))
            {
                dataBaseDir = appPath;
            }
        }
        // mainly Linux: executable placed in something like "bin" subfolder
        else
        {
            if (appPath.length() >= installBinRelPath.length()
                && appPath.rightRef(installBinRelPath.length()) == installBinRelPath)
            {
                // [installed locations]
                // /some/path/to/base_dir/bin/dir [/exename]
                // /some/path_to/base_dir <-> / <-> bin/dir
                dataBaseDir = appPath.left(appPath.length() - (installBinRelPath.length() + 1));
            }
        }

        isInstalled = !dataBaseDir.isEmpty();

        // dev: folder structure not setup yet
        if (!isInstalled)
        {
            dataBaseDir = QDir::currentPath();
            qDebug() << "RuntimeInfo: Assuming to be running in out of source build. Using current working directory as base path.";
        }

#if WIN32
        // In Windows packages and build tree, plugin libraries are always located in a subdirectory of
        // the application location
        binariesBaseDir = appPath;
#else
        // On Linux, plugins are located in hierarchies similar to the data folders
        if (isInstalled)
        {
            binariesBaseDir = dataBaseDir;
        }
        else
        {
            binariesBaseDir = appPath;
        }
#endif
    }

    ~RuntimeInfo_internal() = default;
};

const RuntimeInfo_internal & internalInfos()
{
    return RuntimeInfo_internal::instance();
}

}

const QString & RuntimeInfo::dataPath()
{
#if !WIN32 && !defined(OPTION_LOCAL_INSTALL)
#error Non-local installation is not implemented on the current platform.
#endif

    static const QString path = QDir(internalInfos().dataBaseDir).filePath(config::installDataRelativePath);

    return path;
}

const QString & RuntimeInfo::pluginsPath()
{
#if WIN32
    static const QString path = QDir(internalInfos().binariesBaseDir).filePath(config::installPluginsSharedRelativePath);
#else
    static const QString path = QDir(internalInfos().binariesBaseDir).filePath(
        internalInfos().isInstalled
            ? config::installPluginsSharedRelativePath
            : QString{}    // linux CMake build puts all compiled targets into the build root folder
        );
#endif
    return path;
}
