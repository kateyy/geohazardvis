#include <iostream>
#include <limits>

#if defined(_MSC_VER)
#include <Windows.h>
#elif defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#endif

#include <core/config.h>
#include "Application.h"


namespace
{

#if defined(_MSC_VER)
QString pathToSelf()
{
    // "By convention, argv [0] is the command with which the program is invoked. (...)"
    // E.g., this may be the exe name only (without the path) when invoked from a command line.
    // So get the correct binary path via Win32 API
    TCHAR win_applicationPath[_MAX_PATH + 1];
    const auto pathLength = GetModuleFileName(NULL, win_applicationPath, _MAX_PATH);
    if (pathLength == ERROR_INSUFFICIENT_BUFFER)
    {
        std::cerr << "Cannot determine the application path: insufficient buffer" << std::endl;
        return {};
    }

#if TCHAR == WCHAR
    auto applicationPath = QString::fromWCharArray(win_applicationPath, pathLength);
#else
    auto applicationPath = QString::fromLatin1(win_applicationPath, pathLength);
#endif

    applicationPath.replace('\\', '/');

    return applicationPath;
}

#elif defined(__linux__)
QString pathToSelf()
{
    // http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
    // http://man7.org/linux/man-pages/man2/readlink.2.html
    const char * exeSymLink = "/proc/self/exe";
    struct stat sb;

    ssize_t r;
    if (stat(exeSymLink, &sb) == -1)
    {
        std::cerr << R"(lstat() failed on ")" << exeSymLink << std::endl;
        return {};
    }

    if (sb.st_size + 1 > static_cast<decltype(sb.st_size)>(std::numeric_limits<int>::max()))
    {
        std::cerr << "Path to executable is too long (for Qt)." << std::endl;
        return {};
    }

    QByteArray linkname(static_cast<int>(sb.st_size + 1), '\0');
    r = readlink(exeSymLink, linkname.data(), sb.st_size + 1);

    if (r == -1)
    {
        std::cerr << R"(readlink() failed on ")" << exeSymLink << std::endl;
        return {};
    }

    if (r > sb.st_size)
    {
        std::cerr << "symlink increased in size between lstat() and readlink()" << std::endl;
        return {};
    }

    return QString::fromUtf8(linkname);
}

#else
QString pathToSelf()
{
    return {};
}
#endif

void initializeLibraryPath()
{
    auto applicationPath = pathToSelf();

    if (applicationPath.isEmpty())
    {
        std::cerr << "Could not determin the executable location. Cannot ensure to load correct Qt libraries and plugins." << std::endl;
        return;
    }

    applicationPath.truncate(applicationPath.lastIndexOf('/'));

    // search for libraries deployed with the application first
    QStringList libraryPaths;
    libraryPaths << applicationPath;

    for (auto && path : QCoreApplication::libraryPaths())
    {
        libraryPaths << path;
    }

    QCoreApplication::setLibraryPaths(libraryPaths);
}

}


int main(int argc, char** argv)
{
    initializeLibraryPath();

    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && !defined(__linux__)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

#if VTK_RENDERING_BACKEND == 2
    // Enforce desktop OpenGL, as Angle (OpenGL ES 2.0) and software OpenGL (2.0) are not
    // sufficient for VTK's new OpenGL backend.
    // By default, Qt switches to Angle on some older systems (e.g., Intel HD 3000 / Windows).
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

    QCoreApplication::setApplicationName(config::metaProjectName);

    Application app(argc, argv);

    app.startup();

    return app.exec();
}
