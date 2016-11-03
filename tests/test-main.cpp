#include <QCommandLineParser>
#include <QCoreApplication>

#include <gtest/gtest.h>

#include <TestEnvironment.h>


int main(int argc, char* argv[])
{
    TestEnvironment::init(argc, argv);

    QCommandLineParser cmdParser;
    const QCommandLineOption waitAfterFinishedOption("waitAfterFinished");
    cmdParser.addOption(waitAfterFinishedOption);
    cmdParser.parse(QCoreApplication::instance()->arguments());

    ::testing::InitGoogleTest(&argc, argv);

    auto result = RUN_ALL_TESTS();

    TestEnvironment::release();

    if (cmdParser.isSet(waitAfterFinishedOption))
    {
        getchar();
    }

    return result;
}
