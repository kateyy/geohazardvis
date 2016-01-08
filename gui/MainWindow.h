#pragma once

#include <map>
#include <memory>

#include <QMainWindow>
#include <QVector>

#include <gui/gui_api.h>


template<typename T> class QFutureWatcher;
class QMutex;
class QStringList;
class AbstractRenderView;
class CanvasExporterWidget;
class ColorMappingChooser;
class DataBrowser;
class DataMapping;
class DataSetHandler;
class GlyphMappingChooser;
class GuiPluginManager;
class RenderConfigWidget;
class RendererConfigWidget;
class TableView;
class Ui_MainWindow;


class GUI_API MainWindow : public QMainWindow
{
public:
    MainWindow();
    ~MainWindow() override;

    bool darkFusionStyleEnabled() const;

public:
    void openFiles(const QStringList & fileNames);
    
    void tabbedDockWidgetToFront(QDockWidget * widget);

    void setDarkFusionStyle(bool enabled);

protected:
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private:
    struct FileLoadResults
    {
        QStringList success;
        QStringList notFound;
        QStringList notSupported;
    };

    FileLoadResults openFilesSync(const QStringList & fileNames);
    void prependRecentFiles(const QStringList & filePaths, const QStringList & invalid = QStringList());

    void addRenderView(AbstractRenderView * renderView);
    void addTableView(TableView * tableView, QDockWidget * dockTabifyPartner = nullptr);

    void showDEMWidget();
    void dialog_exportDataSet();
    QStringList dialog_inputFileName();
    void updateWindowTitle();
    void handleAsyncLoadFinished();

    void restoreSettings();
    void storeSettings();
    void restoreUiState();
    void storeUiState();

private:
    QPalette m_defaultPalette;

    std::unique_ptr<Ui_MainWindow> m_ui;
    std::unique_ptr<DataSetHandler> m_dataSetHandler;
    std::unique_ptr<DataMapping> m_dataMapping;
    DataBrowser * m_dataBrowser;
    ColorMappingChooser * m_colorMappingChooser;
    GlyphMappingChooser * m_vectorMappingChooser;
    RenderConfigWidget * m_renderConfigWidget;
    RendererConfigWidget * m_rendererConfigWidget;
    CanvasExporterWidget * m_canvasExporter;

    QVector<QMetaObject::Connection> m_renderViewConnects;

    QString m_lastOpenFolder;
    QString m_lastExportFolder;
    int m_recentFileListMaxEntries;
    QStringList m_recentFileList;

    std::unique_ptr<QMutex> m_loadWatchersMutex;
    std::map<std::unique_ptr<QFutureWatcher<FileLoadResults>>, QStringList> m_loadWatchers;

    std::unique_ptr<GuiPluginManager> m_pluginManager;
};
