#include "application.h"

#include "gui/viewer.h"

Application::Application(int & argc, char ** argv)
: QApplication(argc, argv)
, m_viewer(nullptr)
{
}

void Application::startup()
{
    m_viewer = new Viewer();
    m_viewer->show();
}

Application::~Application()
{
    delete m_viewer;
}