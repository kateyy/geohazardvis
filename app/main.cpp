#include "Application.h"

#if defined(_MSC_VER)
#include <Windows.h>
#endif

#include "config.h"


void initializeLibraryPath();


int main(int argc, char** argv)
{
    initializeLibraryPath();

#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && !defined(__linux__)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QCoreApplication::setApplicationName(metaProjectName());

    Application app(argc, argv);

    app.startup();

    return app.exec();
}

void initializeLibraryPath()
{
#if defined(_MSC_VER)
    // "By convention, argv [0] is the command with which the program is invoked. (...)"
    // E.g., this may be the exe name only (without the path) when invoked from a command line.
    // So get the correct binary path via Win32 API
    TCHAR win_applicationPath[_MAX_PATH + 1];
    auto pathLength = GetModuleFileName(NULL, win_applicationPath, _MAX_PATH);
    if (pathLength == ERROR_INSUFFICIENT_BUFFER)
    {
        printf("Cannot determine the application path: insufficient buffer");
        return;
    }

#if TCHAR == WCHAR
    QString applicationPath = QString::fromWCharArray(win_applicationPath, pathLength);
#else
    QString applicationPath = QString::fromLatin1(win_applicationPath, pathLength);
#endif

    applicationPath.truncate(applicationPath.lastIndexOf('\\'));
    applicationPath.replace('\\', '/');
    if (applicationPath.isEmpty())
    {
        applicationPath = ".";
    }

    // search for libraries deployed with the application first
    QStringList libraryPaths;
    libraryPaths << applicationPath;

    if (!QCoreApplication::libraryPaths().isEmpty())
    {
        libraryPaths << QCoreApplication::libraryPaths().first();
    }

    QCoreApplication::setLibraryPaths(libraryPaths);
#endif
}
