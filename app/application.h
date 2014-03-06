#pragma once

#include <QApplication>

class Viewer;

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int & argc, char ** argv);
    virtual ~Application() override;

    virtual void startup();

protected:
    Viewer * m_viewer;
};