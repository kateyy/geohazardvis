#pragma once

#include <QMainWindow>
#include <QStringList>

#include <gui/gui_api.h>


class DataMapping;
class RenderView;
class RenderConfigWidget;
class RendererConfigWidget;
class DataBrowser;
class ScalarMappingChooser;
class VectorMappingChooser;
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
    void on_actionNew_Render_View_triggered();

    void tabbedDockWidgetToFront(QDockWidget * widget);

    RenderView * addRenderView(int index);

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
    ScalarMappingChooser * m_scalarMappingChooser;
    VectorMappingChooser * m_vectorMappingChooser;
    RenderConfigWidget * m_renderConfigWidget;
    RendererConfigWidget * m_rendererConfigWidget;

    QString m_lastOpenFolder;
};
