#pragma once

#include <memory>

#include <QApplication>


class MainWindow;

class Application : public QApplication
{
public:
    Application(int & argc, char ** argv);

    void startup();

private:
    std::unique_ptr<MainWindow> m_mainWindow;
};