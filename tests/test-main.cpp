#include <QCommandLineParser>
#include <QCoreApplication>

#include <gtest/gtest.h>

#if defined(TEST_WITH_OPENGL_SUPPORT)
#include <gui/data_view/t_QVTKWidget.h>
#endif

#include <TestEnvironment.h>


int main(int argc, char* argv[])
{
#if defined(TEST_WITH_OPENGL_SUPPORT)
    t_QVTKWidget::initializeDefaultSurfaceFormat();
#endif

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
