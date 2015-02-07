#include "Application.h"

#include <QDebug>

#include <gui/MainWindow.h>


Application::Application(int & argc, char ** argv)
: QApplication(argc, argv)
, m_mainWindow(nullptr)
{
}

void Application::startup()
{
    m_mainWindow = new MainWindow();
    m_mainWindow->show();

    QStringList fileNames = arguments();
    // skip the executable path
    fileNames.removeFirst();

    if (!fileNames.isEmpty())
        m_mainWindow->openFiles(fileNames);
}

Application::~Application()
{
    delete m_mainWindow;
}
