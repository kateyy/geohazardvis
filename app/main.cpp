#include "Application.h"


int main(int argc, char** argv)
{
#if defined(WIN32)
    auto applicationPath = QString::fromLocal8Bit(argv[0]);
    applicationPath.truncate(applicationPath.lastIndexOf('\\'));
    applicationPath.replace('\\', '/');

    // search for libraries deployed with the application first
    QStringList libraryPaths;
    libraryPaths << applicationPath;

    if (!QCoreApplication::libraryPaths().isEmpty())
        libraryPaths << QCoreApplication::libraryPaths().first();

    QCoreApplication::setLibraryPaths(libraryPaths);
#endif

    Application app(argc, argv);

    app.startup();

    return app.exec();
}
