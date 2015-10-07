#include "Application.h"

#include <gui/MainWindow.h>


Application::Application(int & argc, char ** argv)
: QApplication(argc, argv)
, m_mainWindow(nullptr)
{
}

Application::~Application() = default;

void Application::startup()
{
    m_mainWindow = std::make_unique<MainWindow>();
    m_mainWindow->show();

    QStringList fileNames = arguments();
    // skip the executable path
    fileNames.removeFirst();

    if (!fileNames.isEmpty())
        m_mainWindow->openFilesAsync(fileNames);
}
