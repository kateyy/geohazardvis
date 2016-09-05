#include "TestEnvironment.h"

#include <cassert>
#include <memory>

#include <QApplication>
#include <QDir>


namespace
{

std::unique_ptr<QApplication> l_app;

const QString & testDataDirName()
{
    static const QString d = "test_data";
    return d;
}

}


void TestEnvironment::init(int argc, char ** argv)
{
    assert(!l_app);
    l_app = std::make_unique<QApplication>(argc, argv);
}

void TestEnvironment::release()
{
    assert(l_app);
    l_app.reset();
}

const QString & TestEnvironment::applicationFilePath()
{
    static const auto p = l_app->applicationFilePath();
    return p;
}

const QString & TestEnvironment::applicationDirPath()
{
    static const auto p = l_app->applicationDirPath();
    return p;
}

const QString & TestEnvironment::testDirPath()
{
    static const auto p = QDir(applicationDirPath()).filePath(testDataDirName());
    return p;
}

void TestEnvironment::createTestDir()
{
    QDir(applicationDirPath()).mkpath(testDataDirName());
}

void TestEnvironment::clearTestDir()
{
    QDir(testDirPath()).removeRecursively();
}
