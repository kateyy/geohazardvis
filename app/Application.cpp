#include "Application.h"

#include "gui/MainWindow.h"


Application::Application(int & argc, char ** argv)
: QApplication(argc, argv)
, m_mainWindow(nullptr)
{
}

void Application::startup()
{
    m_mainWindow = new MainWindow();
    m_mainWindow->show();
}

Application::~Application()
{
    delete m_mainWindow;
}
