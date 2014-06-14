#pragma once

#include <QMainWindow>
#include <QList>

#include <gui/gui_api.h>


class Input;
class DataObject;
class DataMapping;
class RenderWidget;
class RenderConfigWidget;
class DataChooser;
class Ui_MainWindow;


class GUI_API MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

public slots:
    void openFile(QString filename);
    void on_actionOpen_triggered();

    RenderWidget * addRenderWidget(int index);

protected slots:
    void openTable();
    void openRenderView();
    void addToRenderView();

    void updateRenderViewActions(QList<RenderWidget*> widgets);

protected:
    QString dialog_inputFileName();

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

    DataObject * selectedDataObject();

protected:
    Ui_MainWindow * m_ui;
    DataMapping * m_dataMapping;
    QAction * m_addToRendererAction;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;

    QString m_lastOpenFolder;

    QList<DataObject *> m_dataObjects;
};
