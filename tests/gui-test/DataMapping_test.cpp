#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QPointer>
#include <QVBoxLayout>

#include <core/DataSetHandler.h>
#include <gui/DataMapping.h>
#include <gui/data_view/AbstractRenderView.h>


class DataMapping_test : public testing::Test
{
public:
    void SetUp() override
    {
        env = std::make_unique<TestEnv>();
    }
    void TearDown() override
    {
        env.reset();
    }

    struct TestEnv
    {
        DataSetHandler dataSetHander;
        DataMapping mapping;
        TestEnv()
            : dataSetHander{}
            , mapping(dataSetHander)
        {
        }
    };
    std::unique_ptr<TestEnv> env;
};

TEST_F(DataMapping_test, RenderViewDeletedAfterClose)
{
    QPointer<AbstractRenderView> view = env->mapping.createDefaultRenderViewType();
    ASSERT_TRUE(view);

    view->close();
    QCoreApplication::processEvents();

    ASSERT_FALSE(view);
}

TEST_F(DataMapping_test, ClosedRenderViewValidUntilEventLoop)
{
    QPointer<AbstractRenderView> view = env->mapping.createDefaultRenderViewType();
    ASSERT_TRUE(view);

    view->close();
    ASSERT_TRUE(view);
    QCoreApplication::processEvents();

    ASSERT_FALSE(view);
}

TEST_F(DataMapping_test, ViewOwnershipPassedToQt)
{
    QPointer<AbstractRenderView> view = env->mapping.createDefaultRenderViewType();
    ASSERT_TRUE(view);

    {
        QWidget parent;
        auto layout = new QVBoxLayout();
        layout->addWidget(view);
        // Qt takes ownership here
        parent.setLayout(layout);
        ASSERT_EQ(&parent, view->parent());
    }

    // must be deleted here, as it was owned by Qt
    ASSERT_FALSE(view);
}

TEST_F(DataMapping_test, ViewOwnershipPassedToMainWindow)
{
    QPointer<AbstractRenderView> view = env->mapping.createDefaultRenderViewType();
    ASSERT_TRUE(view);

    QMainWindow mainWindow;

    mainWindow.addDockWidget(Qt::LeftDockWidgetArea, view->dockWidgetParent());
    ASSERT_EQ(view->dockWidgetParent(), view->parent());
    ASSERT_EQ(&mainWindow, view->dockWidgetParent()->parent());
    view->close();

    ASSERT_TRUE(view);

    QCoreApplication::processEvents();

    ASSERT_FALSE(view);
}

TEST_F(DataMapping_test, ViewDeletedOnAppShutdown)
{
    QPointer<AbstractRenderView> view = env->mapping.createDefaultRenderViewType();
    ASSERT_TRUE(view);

    {
        QMainWindow mainWindow;

        mainWindow.addDockWidget(Qt::LeftDockWidgetArea, view->dockWidgetParent());
        ASSERT_EQ(view->dockWidgetParent(), view->parent());
        ASSERT_EQ(&mainWindow, view->dockWidgetParent()->parent());

        env.reset();
        ASSERT_FALSE(view);
    }
}

TEST_F(DataMapping_test, FocusRenderViewAfterCreate)
{
    auto view = env->mapping.createDefaultRenderViewType();
    ASSERT_EQ(view, env->mapping.focusedRenderView());
}

TEST_F(DataMapping_test, SwitchFocusToNewRenderView)
{
    /*auto view1 =*/ env->mapping.createDefaultRenderViewType();
    auto view2 = env->mapping.createDefaultRenderViewType();
    ASSERT_EQ(view2, env->mapping.focusedRenderView());
}

TEST_F(DataMapping_test, SwitchFocusAfterRenderViewClose)
{
    auto view1 = env->mapping.createDefaultRenderViewType();
    auto view2 = env->mapping.createDefaultRenderViewType();
    ASSERT_EQ(view2, env->mapping.focusedRenderView());
    view2->close();
    ASSERT_EQ(view1, env->mapping.focusedRenderView());
}

TEST_F(DataMapping_test, ResetFocusAfterAllRenderViewsClosed)
{
    auto view1 = env->mapping.createDefaultRenderViewType();
    auto view2 = env->mapping.createDefaultRenderViewType();

    view1->close();
    view2->close();

    ASSERT_EQ(nullptr, env->mapping.focusedRenderView());
}
