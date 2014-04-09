#pragma once

#include <QList>

#include <QMainWindow>

class Ui_MainWindow;
class InputViewer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

public slots:
    void on_actionOpen_triggered();

protected:
    Ui_MainWindow * m_ui;

    QList<InputViewer *> m_inputViewers;

};
