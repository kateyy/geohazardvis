#pragma once

#include <memory>

#include <QMainWindow>


class SelectionHandler;
class Input;
class InputRepresentation;
class RenderView;
class RenderConfigWidget;
class TableWidget;
class DataChooser;
class Ui_MainWindow;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

public slots:
    void openFile(QString filename);
    void on_actionOpen_currentTab_triggered();
    void on_actionOpen_newTab_triggered();

protected:
    QString dialog_inputFileName();

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

protected:
    Ui_MainWindow * m_ui;
    RenderView * m_renderView;
    TableWidget * m_tableWidget;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;

    QString m_lastOpenFolder;

    std::list<std::shared_ptr<InputRepresentation>> m_inputs;
};
