#pragma once

#include <QMainWindow>
#include <QStringList>

#include <gui/gui_api.h>


class CanvasExporterWidget;
class DataBrowser;
class DataMapping;
class RenderConfigWidget;
class RenderView;
class RendererConfigWidget;
class ColorMappingChooser;
class Ui_MainWindow;
class GlyphMappingChooser;


class GUI_API MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

public slots:
    void openFiles(QStringList fileNames);

    void on_actionOpen_triggered();
    void on_actionExportDataset_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionNew_Render_View_triggered();

    void tabbedDockWidgetToFront(QDockWidget * widget);

    void addRenderView(RenderView * renderView);

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
    ColorMappingChooser * m_scalarMappingChooser;
    GlyphMappingChooser * m_vectorMappingChooser;
    RenderConfigWidget * m_renderConfigWidget;
    RendererConfigWidget * m_rendererConfigWidget;
    CanvasExporterWidget * m_canvasExporter;

    QString m_lastOpenFolder;
    QString m_lastExportFolder;
};
