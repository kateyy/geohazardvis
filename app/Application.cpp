#include "Application.h"

#include <QFileInfo>
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

    QStringList fileNames;
    
    // skip the executable path
    for (int i = 1; i < arguments().size(); ++i)
        if (QFileInfo(arguments()[i]).exists())
            fileNames << arguments()[i];

    if (!fileNames.isEmpty())
        m_mainWindow->openFiles(fileNames);
}

Application::~Application()
{
    delete m_mainWindow;
}
