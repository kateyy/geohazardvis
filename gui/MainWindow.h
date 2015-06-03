#pragma once

#include <QMainWindow>
#include <QMap>

#include <gui/gui_api.h>


template<typename T> class QFutureWatcher;
class QMutex;
class QStringList;
class vtkQtDebugLeaksView;
class AbstractRenderView;
class CanvasExporterWidget;
class DataBrowser;
class DataMapping;
class RenderConfigWidget;
class RendererConfigWidget;
class ColorMappingChooser;
class Ui_MainWindow;
class GlyphMappingChooser;


class GUI_API MainWindow : public QMainWindow
{
public:
    MainWindow();
    ~MainWindow() override;

    bool darkFusionStyleEnabled() const;

public:
    void openFiles(const QStringList & fileNames);
    void openFilesAsync(const QStringList & fileNames);
    
    void addRenderView(AbstractRenderView * renderView);
    void tabbedDockWidgetToFront(QDockWidget * widget);

    void setDarkFusionStyle(bool enabled);

protected:
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private:
    void showDEMWidget();
    void dialog_exportDataSet();
    QStringList dialog_inputFileName();
    void updateWindowTitle();
    void handleAsyncLoadFinished();

private:
    vtkQtDebugLeaksView * m_debugLeaksView;
    QPalette m_defaultPalette;

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

    QMutex * m_loadWatchersMutex;
    QMap<QFutureWatcher<void> *, QStringList> m_loadWatchers;
};
