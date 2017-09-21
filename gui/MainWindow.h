/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <QMainWindow>

#include <gui/DataMapping.h>


class QByteArray;
template<typename T> class QFutureWatcher;
class QMutex;
class QStringList;
class AbstractRenderView;
class CanvasExporterWidget;
class ColorMappingChooser;
class DataBrowser;
class DataSetHandler;
class GlyphMappingChooser;
class GuiPluginManager;
class RenderPropertyConfigWidget;
class RendererConfigWidget;
class TableView;
class Ui_MainWindow;


class GUI_API MainWindow : public QMainWindow
{
public:
    MainWindow();
    ~MainWindow() override;

    bool darkFusionStyleEnabled() const;

    void openFiles(const QStringList & fileNames);
    
    void tabbedDockWidgetToFront(QDockWidget * widget);

    void setDarkFusionStyle(bool enabled);

    /** @return The data mapping that allows to represent loaded data in table and render views.
      * The data views will be docked to this MainWindow.
      * Use dataMapping().dataSetHandler() to access current data objects, pass them to the data
      * mapping or load/delete stored data. */
    DataMapping & dataMapping();
    const DataMapping & dataMapping() const;

    void addPluginMenuAction(QAction * action);
    void removePluginMenuAction(QAction * action);

protected:
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dropEvent(QDropEvent * event) override;

private:
    struct FileLoadResults
    {
        QList<DataObject *> newData;
        QStringList success;
        QStringList notFound;
        QStringList notSupported;
    };

    FileLoadResults openFilesSync(const QStringList & fileNames);
    void prependRecentFiles(const QStringList & filePaths, const QStringList & invalid = QStringList());

    void addRenderView(AbstractRenderView * renderView, DataMapping::OpenFlags openFlags);
    void addTableView(TableView * tableView, QDockWidget * dockTabifyPartner = nullptr);

    void showDEMWidget();
    void dialog_exportDataSet();
    void dialog_exportToCSV();
    QStringList dialog_inputFileName();
    void updateWindowTitle();
    void handleAsyncLoadFinished();

    void restoreStyle();
    void storeStyle();
    void restoreSettings();
    void storeSettings();
    void restoreUiState();
    void storeUiState();
    void resetUiStateToDefault();

private:
    std::unique_ptr<Ui_MainWindow> m_ui;
    std::pair<QByteArray, QByteArray> m_defaultUiState;
    std::unique_ptr<DataSetHandler> m_dataSetHandler;
    std::unique_ptr<DataMapping> m_dataMapping;
    DataBrowser * m_dataBrowser;
    ColorMappingChooser * m_colorMappingChooser;
    GlyphMappingChooser * m_vectorMappingChooser;
    RenderPropertyConfigWidget * m_renderPropertyConfigWidget;
    RendererConfigWidget * m_rendererConfigWidget;
    CanvasExporterWidget * m_canvasExporter;

    std::vector<QMetaObject::Connection> m_renderViewConnects;

    QString m_lastOpenFolder;
    QString m_lastExportFolder;
    int m_recentFileListMaxEntries;
    QStringList m_recentFileList;

    std::unique_ptr<QMutex> m_loadWatchersMutex;
    std::map<std::unique_ptr<QFutureWatcher<FileLoadResults>>, QStringList> m_loadWatchers;

    std::unique_ptr<GuiPluginManager> m_pluginManager;

private:
    Q_DISABLE_COPY(MainWindow)
};
