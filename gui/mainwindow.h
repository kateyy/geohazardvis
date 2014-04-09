#pragma once

#include <QMainWindow>

class Ui_MainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

    QWidget * dockSpace() const;

public slots:
    void on_actionOpen_triggered();

protected:
    Ui_MainWindow * m_ui;
};
