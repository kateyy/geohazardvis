#pragma once

#include <QMainWindow>
#include <QList>
#include <QStringList>

#include <gui/gui_api.h>


class Input;
class DataObject;
class DataMapping;
class RenderView;
class RenderConfigWidget;
class DataBrowser;
class DataChooser;
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
    void on_actionAbout_Qt_triggered();

    void tabbedDockWidgetToFront(QDockWidget * widget);

    RenderView * addRenderView(int index);

private slots:
    //void updateRenderViewActions(QList<RenderWidget*> widgets);

private:
    QStringList dialog_inputFileName();

    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private:
    Ui_MainWindow * m_ui;
    DataMapping * m_dataMapping;
    QAction * m_addToRendererAction;
    QAction * m_removeLoadedFileAction;
    DataBrowser * m_dataBrowser;
    DataChooser * m_dataChooser;
    RenderConfigWidget * m_renderConfigWidget;

    QString m_lastOpenFolder;
};
