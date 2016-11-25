#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QtConcurrent/QtConcurrentRun>

#include <core/ApplicationSettings.h>
#include <core/DataSetHandler.h>
#include <core/RuntimeInfo.h>
#include <core/VersionInfo.h>
#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/io/Exporter.h>
#include <core/io/Loader.h>
#include <core/rendered_data/RenderedData.h>
#include <core/ThirdParty/dark_fusion_style.hpp>
#include <core/utility/qthelper.h>

#include <gui/DataMapping.h>
#include <gui/SelectionHandler.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/data_view/TableView.h>
#include <gui/plugin/GuiPluginInterface.h>
#include <gui/plugin/GuiPluginManager.h>
#include <gui/widgets/CoordinateSystemAdjustmentWidget.h>
#include <gui/widgets/DataImporterWidget.h>
#include <gui/widgets/GridDataImporterWidget.h>
#include <gui/widgets/CanvasExporterWidget.h>
#include <gui/widgets/ColorMappingChooser.h>
#include <gui/widgets/DEMWidget.h>
#include <gui/widgets/GlyphMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>
#include <gui/widgets/RendererConfigWidget.h>

#include "config.h"

#if OPTION_ENABLE_TEXTURING
#include <core/TextureManager.h>
#endif


MainWindow::MainWindow()
    : QMainWindow()
    , m_ui{ std::make_unique<Ui_MainWindow>() }
    , m_dataSetHandler{ std::make_unique<DataSetHandler>() }
    , m_dataMapping{}
    , m_colorMappingChooser{ new ColorMappingChooser() }
    , m_vectorMappingChooser{ new GlyphMappingChooser() }
    , m_renderConfigWidget{ new RenderConfigWidget() }
    , m_rendererConfigWidget{ new RendererConfigWidget() }
    , m_canvasExporter{ new CanvasExporterWidget(this) }
    , m_recentFileListMaxEntries{ 0 }
    , m_loadWatchersMutex{ std::make_unique<QMutex>() }
{
    m_ui->setupUi(this);
    m_ui->actionOpen->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton));
    m_ui->actionExportDataset->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_ui->actionExit->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCloseButton));

    restoreStyle();

    m_dataMapping = std::make_unique<DataMapping>(*m_dataSetHandler);
    connect(m_dataMapping.get(), &DataMapping::renderViewCreated, this, &MainWindow::addRenderView);
    connect(m_dataMapping.get(), &DataMapping::tableViewCreated, this, &MainWindow::addTableView);

#if OPTION_ENABLE_TEXTURING
    TextureManager::initialize();
#endif

    connect(m_ui->actionOpen, &QAction::triggered, [this] () { openFiles(dialog_inputFileName()); });
    connect(m_ui->actionImport_CSV_Triangle_Mesh, &QAction::triggered, [this] () {
        DataImporterWidget importer(this);
        if (importer.exec() == QDialog::Accepted)
        {
            m_dataSetHandler->takeData(importer.releaseLoadedData());
        }
    });
    connect(m_ui->actionImport_CSV_Grid_Data, &QAction::triggered, [this] () {
        GridDataImporterWidget importer(this);
        if (importer.exec() == QDialog::Accepted)
        {
            m_dataSetHandler->takeData(importer.releaseLoadedData());
        }
    });
    connect(m_ui->actionExportDataset, &QAction::triggered, this, &MainWindow::dialog_exportDataSet);
    connect(m_ui->actionClear_Recent_Files, &QAction::triggered, [this] () {
        m_recentFileList.clear();
        prependRecentFiles({});
    });
    connect(m_ui->actionAbout, &QAction::triggered, [this] () {
        auto && projectInfo = VersionInfo::projectInfo();
        auto infoString = QString(
            "Maintainer contact:\n"
            "\t%1\n"
            "\n"
            "Project Version: %2.%3.%4\n"
            "\tRevision: %5\n"
            "\tDate: %6\n"
            "\n"
            "Third Party Libraries:\n")
            .arg(projectInfo.maintainerEmail)
            .arg(projectInfo.major)
            .arg(projectInfo.minor)
            .arg(projectInfo.patch)
            .arg(projectInfo.gitRevision)
            .arg(projectInfo.gitCommitDate.date().toString());
        for (auto && thirdParty : VersionInfo::supportedThirdParties())
        {
            auto && info = VersionInfo::versionInfoFor(thirdParty);
            infoString += QString(
                "%1 (Version %2.%3.%4)\n"
                "\tRevision: %5\n"
                "\tDate: %6\n")
                .arg(thirdParty)
                .arg(info.major)
                .arg(info.minor)
                .arg(info.patch)
                .arg(info.gitRevision)
                .arg(info.gitCommitDate.date().toString());
        }
        infoString += QString(R"(Plese refer to the ")") + config::installThirdPartyLicensePath +
            R"(" folder in the appplication folder for further information and licenses.)";
        QMessageBox::about(this, config::metaProjectName, infoString);
    });
    connect(m_ui->actionAbout_Qt, &QAction::triggered, [this] () { QMessageBox::aboutQt(this); });
    connect(m_ui->actionApply_Digital_Elevation_Model, &QAction::triggered, this, &MainWindow::showDEMWidget);
    connect(m_ui->actionAdjust_Coordinate_System, &QAction::triggered, [this] ()
    {
        auto && selection = m_dataBrowser->selectedDataSets();
        if (selection.isEmpty())
        {
            QMessageBox::information(this, "Coordinate System Specification",
                "Please select a data object in the data browser, first!");
            return;
        }
        auto transformableDataObject = dynamic_cast<CoordinateTransformableDataObject *>(selection.first());
        if (!transformableDataObject)
        {
            QMessageBox::information(this, "Coordinate System Specification",
                "The coordinate system of the selected data object cannot be changed (" ").");
            return;
        }

        CoordinateSystemAdjustmentWidget dialog(this);
        dialog.setDataObject(transformableDataObject);
        dialog.exec();
    });
    connect(m_ui->actionNew_Render_View, &QAction::triggered,
        m_dataMapping.get(), &DataMapping::createDefaultRenderViewType);
    connect(m_ui->actionExit, &QAction::triggered, qApp, &QApplication::quit);


    m_dataBrowser = m_ui->centralwidget;
    m_dataBrowser->setDataMapping(m_dataMapping.get());

    m_dataMapping->selectionHandler().setSyncToggleMenu(m_ui->menuSynchronize_Selections);

    setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
    setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_colorMappingChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_vectorMappingChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_rendererConfigWidget);
    tabifyDockWidget(m_renderConfigWidget, m_vectorMappingChooser);
    tabifyDockWidget(m_renderConfigWidget, m_rendererConfigWidget);
    tabbedDockWidgetToFront(m_renderConfigWidget);

    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_colorMappingChooser, &ColorMappingChooser::setCurrentRenderView);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_vectorMappingChooser, &GlyphMappingChooser::setCurrentRenderView);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_renderConfigWidget, &RenderConfigWidget::setCurrentRenderView);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_rendererConfigWidget, &RendererConfigWidget::setCurrentRenderView);

    connect(m_dataBrowser, &DataBrowser::selectedDataChanged,
        [this] (DataObject * selected) {
        m_renderConfigWidget->setSelectedData(selected);
        m_colorMappingChooser->setSelectedData(selected);
        m_vectorMappingChooser->setSelectedData(selected);
    });

    connect(m_ui->actionSetup_Image_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::show);
    connect(m_ui->actionQuick_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshot);
    connect(m_ui->actionOpen_Export_Folder, &QAction::triggered, [this] () {
        // check: https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
        auto path = QFileInfo(m_canvasExporter->currentExportFolder()).absoluteFilePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    connect(m_ui->actionExport_To, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshotTo);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_canvasExporter, &CanvasExporterWidget::setRenderView);

    connect(m_ui->actionResidual_Verification_View, &QAction::triggered,
        [this] (bool) {
        auto view = m_dataMapping->createRenderView<ResidualVerificationView>();
        m_dataMapping->setFocusedRenderView(view);
    });

    connect(m_ui->actionDark_Style, &QAction::triggered, this, &MainWindow::setDarkFusionStyle);

    m_ui->menuViews->insertAction(m_ui->actionDark_Style, m_colorMappingChooser->toggleViewAction());
    m_ui->menuViews->insertAction(m_ui->actionDark_Style, m_vectorMappingChooser->toggleViewAction());
    m_ui->menuViews->insertAction(m_ui->actionDark_Style, m_renderConfigWidget->toggleViewAction());
    m_ui->menuViews->insertAction(m_ui->actionDark_Style, m_rendererConfigWidget->toggleViewAction());
    m_ui->menuViews->insertSeparator(m_ui->actionDark_Style);

    restoreSettings();

    // load plug-ins

    m_pluginManager = std::make_unique<GuiPluginManager>();
    m_pluginManager->searchPaths() = QStringList(RuntimeInfo::pluginsPath());
    m_pluginManager->scan(GuiPluginInterface(*this, *m_dataMapping));

    restoreUiState();
}

MainWindow::~MainWindow()
{
    while (true)
    {
        decltype(m_loadWatchers)::key_type::pointer watcher;
        {
            QMutexLocker lock(m_loadWatchersMutex.get());
            if (m_loadWatchers.empty())
            {
                break;
            }

            watcher = m_loadWatchers.begin()->first.get();
        }
        watcher->waitForFinished();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    storeUiState();
    storeSettings();
    storeStyle();

    m_pluginManager.reset();

    disconnectAll(m_renderViewConnects);

    m_dataMapping.reset();

    m_ui.reset();

#if OPTION_ENABLE_TEXTURING
    TextureManager::release();
#endif
}

QStringList MainWindow::dialog_inputFileName()
{
    const auto fileNames = QFileDialog::getOpenFileNames(this, "", m_lastOpenFolder, Loader::fileFormatFilters());

    if (fileNames.isEmpty())
    {
        return {};
    }

    m_lastOpenFolder = QFileInfo(fileNames.first()).absolutePath();

    return fileNames;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent * event)
{
    assert(event->mimeData()->hasUrls());

    event->acceptProposedAction();

    QStringList fileNames;
    for (const QUrl & url : event->mimeData()->urls())
    {
        fileNames << url.toLocalFile();
    }

    openFiles(fileNames);
}

void MainWindow::addRenderView(AbstractRenderView * renderView)
{
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, renderView->dockWidgetParent());

    m_renderViewConnects << connect(renderView, &AbstractDataView::selectionChanged,
        [this] (AbstractDataView * renderView, const DataSelection & selection)
    {
        if (renderView == m_dataMapping->focusedRenderView())
        {
            m_dataBrowser->setSelectedData(selection.dataObject);
        }
    });
}

void MainWindow::addTableView(TableView * tableView, QDockWidget * dockTabifyPartner)
{
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, tableView->dockWidgetParent());

    if (dockTabifyPartner)
    {
        tabifyDockWidget(dockTabifyPartner, tableView->dockWidgetParent());
    }

    QCoreApplication::processEvents(); // setup GUI before searching for the tabbed widget...
    tabbedDockWidgetToFront(tableView->dockWidgetParent());
}

MainWindow::FileLoadResults MainWindow::openFilesSync(const QStringList & fileNames)
{
    QString oldName = windowTitle();

    FileLoadResults results;

    std::vector<std::unique_ptr<DataObject>> newData;

    for (auto && fileName : fileNames)
    {
        qDebug() << " <" << fileName;

        if (!QFileInfo(fileName).exists())
        {
            qDebug() << "\t\t File does not exist!";
            results.notFound << fileName;
            continue;
        }

        auto dataObject = Loader::readFile(fileName);
        if (!dataObject)
        {
            results.notSupported << fileName;

            continue;
        }

        newData.push_back(std::move(dataObject));
        results.success << fileName;
    }

    m_dataSetHandler->takeData(std::move(newData));

    return results;
}

void MainWindow::prependRecentFiles(const QStringList & filePaths, const QStringList & invalid)
{
    auto & menu = *m_ui->menuRecent_Files;
    menu.removeAction(m_ui->actionClear_Recent_Files);
    menu.clear();

    auto oldEntries = m_recentFileList;
    m_recentFileList = filePaths.mid(0, m_recentFileListMaxEntries);

    for (auto && oldEntry : oldEntries)
    {
        if (m_recentFileList.size() == m_recentFileListMaxEntries)
        {
            break;
        }

        if (m_recentFileList.contains(oldEntry) || invalid.contains(oldEntry))
        {
            continue;
        }

        m_recentFileList << oldEntry;
    }


    auto openRecentFile = [this] (int i) {
        auto && fileName = m_recentFileList[i];
        openFiles({ fileName });
    };

    int i = 0;

    for (auto && newEntry : m_recentFileList)
    {
        QString num;
        if (i < 9)
        {
            num = "&" + QString::number(i + 1);
        }
        else if (i == 9)
        {
            num = "1&0";
        }
        else
        {
            num = QString::number(i + 1);
        }

        auto action = menu.addAction(num + ": " + newEntry);
        connect(action, &QAction::triggered, std::bind(openRecentFile, i));
        ++i;
    }

    menu.addSeparator();
    menu.addAction(m_ui->actionClear_Recent_Files);
    m_ui->actionClear_Recent_Files->setEnabled(m_recentFileList.size() > 0);
}

void MainWindow::openFiles(const QStringList & fileNames)
{
    auto watcher = std::make_unique<QFutureWatcher<FileLoadResults>>();
    auto watcherPtr = watcher.get();
    connect(watcherPtr, &QFutureWatcher<FileLoadResults>::finished, this, &MainWindow::handleAsyncLoadFinished);

    m_loadWatchersMutex->lock();
    m_loadWatchers.emplace(std::move(watcher), fileNames);
    m_loadWatchersMutex->unlock();

    watcherPtr->setFuture(
        QtConcurrent::run(this, &MainWindow::openFilesSync, fileNames));

    updateWindowTitle();
    QApplication::processEvents();
}

void MainWindow::dialog_exportDataSet()
{
    auto toExport = m_dataBrowser->selectedDataObjects();
    if (toExport.isEmpty())
    {
        QMessageBox::information(this, "Dataset Export", "Please select datasets to export in the data browser!");
        return;
    }

    QString oldName = windowTitle();

    for (auto d : toExport)
    {
        auto & dataObject = *d;
        bool isSupported = Exporter::isExportSupported(dataObject);
        if (!isSupported)
        {
            QMessageBox::warning(this, "Unsupported Operation",
                "Exporting is currently not supported for the selected data type (" + dataObject.dataTypeName() + ")\n"
                + "Data set: """ + dataObject.name() + """");
            continue;
        }

        const auto fileName = QFileDialog::getSaveFileName(this, "", m_lastExportFolder + "/" + dataObject.name(),
            Exporter::formatFilter(dataObject));

        if (fileName.isEmpty())
        {
            continue;
        }

        setWindowTitle("Exporting " + QFileInfo(fileName).baseName() + " ...");
        QApplication::processEvents();

        const bool result = Exporter::exportData(dataObject, fileName);

        if (!result)
        {
            QMessageBox::warning(this, "Export failed",
                "Could not export the specified data set (" + dataObject.name() + ")");
        }

        m_lastExportFolder = QFileInfo(fileName).absolutePath();
    }

    setWindowTitle(oldName);
}

void MainWindow::showDEMWidget()
{
    auto demWidget = new DEMWidget(*m_dataMapping);
    demWidget->setAttribute(Qt::WA_DeleteOnClose);
    auto dockWidget = demWidget->dockWidgetParent();
    dockWidget->setParent(this);
    dockWidget->setFloating(true);
    dockWidget->show();
}

void MainWindow::tabbedDockWidgetToFront(QDockWidget * widget)
{
    // http://qt-project.org/faq/answer/how_can_i_check_which_tab_is_the_current_one_in_a_tabbed_qdockwidget
    QList<QTabBar*> tabBars = findChildren<QTabBar*>("", Qt::FindChildOption::FindDirectChildrenOnly);

    for (QTabBar * tabBar : tabBars)
    {
        for (int i = 0; i < tabBar->count(); ++i)
        {
            bool ok;
            QDockWidget * tabbedWidget = reinterpret_cast<QDockWidget*>(tabBar->tabData(i).toULongLong(&ok));
            assert(ok);

            if (tabbedWidget == widget)
            {
                tabBar->setCurrentIndex(i);
                return;
            }
        }
    }
}

bool MainWindow::darkFusionStyleEnabled() const
{
    return m_ui->actionDark_Style->isChecked();
}

void MainWindow::setDarkFusionStyle(bool enabled)
{
    if (enabled)
    {
        widgetzeug::enableDarkFusionStyle();
    }
    else
    {
#ifdef _WINDOWS
        qApp->setStyle(QStyleFactory::create("windowsvista"));
#else
        qApp->setStyle(QStyleFactory::create("gtk"));
#endif
        qApp->setStyleSheet("");
        qApp->setPalette(qApp->style()->standardPalette());
    }

    m_ui->actionDark_Style->setChecked(enabled);
}

DataMapping & MainWindow::dataMapping()
{
    return *m_dataMapping;
}

const DataMapping & MainWindow::dataMapping() const
{
    return *m_dataMapping;
}

void MainWindow::updateWindowTitle()
{
    QMutexLocker lock(m_loadWatchersMutex.get());

    QString title = config::metaProjectName;

    if (!m_loadWatchers.empty())
    {
        title += " -  Loading Files: ";

        for (auto & it : m_loadWatchers)
        {
            for (auto & fileName : it.second)
            {
                title += QFileInfo(fileName).fileName() + ", ";
            }
        }

        title.truncate(title.length() - 2);
    }

    setWindowTitle(title);
}

void MainWindow::handleAsyncLoadFinished()
{
    {
        QMutexLocker lock(m_loadWatchersMutex.get());
        using WatcherPtr = decltype(m_loadWatchers)::key_type::pointer;
        assert(dynamic_cast<WatcherPtr>(sender()));
        auto watcher = static_cast<WatcherPtr>(sender());

        decltype(m_loadWatchers)::iterator toDeleteIt;
        for (toDeleteIt = m_loadWatchers.begin(); toDeleteIt != m_loadWatchers.end(); ++toDeleteIt)
        {
            if (toDeleteIt->first.get() == watcher)
            {
                break;
            }
        }
        assert(toDeleteIt != m_loadWatchers.end());

        auto results = toDeleteIt->first->future().result();

        prependRecentFiles(results.success, results.notFound + results.notSupported);

        if (results.notFound.size() + results.notSupported.size())
        {
            QString msg = "Could not open the following files:";
            if (results.notSupported.size())
            {
                msg += "\nUnsupported format:";
                for (auto && file : results.notSupported)
                {
                    msg += "\n - " + file;
                }
            }
            if (results.notFound.size())
            {
                msg += "\nFile not found:";
                for (auto && file : results.notFound)
                {
                    msg += "\n - " + file;
                }
            }
            QMessageBox::critical(this, "File error", msg);
        }

        m_loadWatchers.erase(toDeleteIt);
    }

    updateWindowTitle();
}

void MainWindow::restoreStyle()
{
    setDarkFusionStyle(ApplicationSettings().value("darkFusionStyle").toBool());
}

void MainWindow::storeStyle()
{
    ApplicationSettings().setValue("darkFusionStyle", darkFusionStyleEnabled());
}

void MainWindow::restoreSettings()
{
    ApplicationSettings settings;
    m_lastOpenFolder = settings.value("lastOpenFolder").toString();
    m_lastExportFolder = settings.value("lastExportFolder").toString();
    m_recentFileListMaxEntries = std::max(0, settings.value("recentFileListMaxEntries", 15).toInt());
    prependRecentFiles(settings.value("recentFileList").toStringList());
}

void MainWindow::storeSettings()
{
    ApplicationSettings settings;
    settings.setValue("lastOpenFolder", m_lastOpenFolder);
    settings.setValue("lastExportFolder", m_lastExportFolder);
    settings.setValue("recentFileListMaxEntries", m_recentFileListMaxEntries);
    settings.setValue("recentFileList", m_recentFileList);
}

void MainWindow::restoreUiState()
{
    if (!ApplicationSettings::settingsExist())
    {
        this->setWindowState(Qt::WindowState::WindowMaximized);
        return;
    }

    ApplicationSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::storeUiState()
{
    ApplicationSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}
