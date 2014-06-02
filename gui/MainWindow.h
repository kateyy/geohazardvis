#pragma once

#include <memory>

#include <QMainWindow>
#include <QList>


class SelectionHandler;
class Input;
class InputRepresentation;
class DataMapping;
class RenderWidget;
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

    RenderWidget * addRenderWidget();

protected slots:
    void openTable();
    void openRenderView();
    void addToRenderView();

protected:
    QString dialog_inputFileName();

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

    std::shared_ptr<InputRepresentation> selectedInput();

protected:
    Ui_MainWindow * m_ui;
    DataMapping * m_dataMapping;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;
    std::shared_ptr<SelectionHandler> m_selectionHandler;

    QString m_lastOpenFolder;

    QList<std::shared_ptr<InputRepresentation>> m_inputs;
};
