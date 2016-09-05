#pragma once


class QString;


class TestEnvironment
{
public:
    static void init(int argc, char ** argv);
    static void release();

    static const QString & applicationFilePath();
    static const QString & applicationDirPath();

    static const QString & testDirPath();
    static void createTestDir();
    static void clearTestDir();

private:
    TestEnvironment() = delete;
    ~TestEnvironment() = delete;
};
