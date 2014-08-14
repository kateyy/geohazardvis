#pragma once

#include <QMainWindow>
#include <QList>
#include <QStringList>

#include <gui/gui_api.h>


class Input;
class DataObject;
class DataMapping;
class RenderWidget;
class RenderConfigWidget;
class DataChooser;
class LoadedFilesTableModel;
class Ui_MainWindow;


class GUI_API MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

public slots:
    void openFile(QString fileName);
    void openFiles(QStringList fileNames);
    void on_actionOpen_triggered();

    RenderWidget * addRenderWidget(int index);

protected slots:
    void openTable();
    void openRenderView();
    void addToRenderView();

    void updateRenderViewActions(QList<RenderWidget*> widgets);

protected:
    QStringList dialog_inputFileName();

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

    DataObject * selectedDataObject();

protected:
    Ui_MainWindow * m_ui;
    LoadedFilesTableModel * m_loadedFilesModel;
    DataMapping * m_dataMapping;
    QAction * m_addToRendererAction;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;

    QString m_lastOpenFolder;

    QList<DataObject *> m_dataObjects;
};
