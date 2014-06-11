#pragma once

#include <QApplication>

class MainWindow;

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int & argc, char ** argv);
    virtual ~Application() override;

    virtual void startup();

protected:
    MainWindow * m_mainWindow;
};