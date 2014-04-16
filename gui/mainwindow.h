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
    void on_actionOpen_currentTab_triggered();
    void on_actionOpen_newTab_triggered();

protected slots:
    void viewerTitleChanged(const QString & title);

protected:
    QString dialog_inputFileName();

protected:
    Ui_MainWindow * m_ui;

    QList<InputViewer *> m_inputViewers;

    QString m_lastOpenFolder;

};
