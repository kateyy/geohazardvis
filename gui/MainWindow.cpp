#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QDebug>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QMessageBox>
#include <QMimeData>
#include <QtConcurrent/QtConcurrentRun>

//#include <vtkQtDebugLeaksView.h>

#include <widgetzeug/dark_fusion_style.hpp>

#include <core/DataSetHandler.h>
#include <core/TextureManager.h>
#include <core/data_objects/DataObject.h>
#include <core/io/Exporter.h>
#include <core/io/Loader.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/SelectionHandler.h>
#include <gui/data_view/AbstractRenderView.h>
#include <gui/data_view/ResidualVerificationView.h>
#include <gui/widgets/CanvasExporterWidget.h>
#include <gui/widgets/ColorMappingChooser.h>
#include <gui/widgets/DEMWidget.h>
#include <gui/widgets/GlyphMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>
#include <gui/widgets/RendererConfigWidget.h>

namespace
{
    const char s_defaultAppTitle[] = "GeohazardVis";
}

MainWindow::MainWindow()
    : QMainWindow()
    //, m_debugLeaksView(new vtkQtDebugLeaksView()) // not usable in multi-threaded application
    , m_ui(std::make_unique<Ui_MainWindow>())
    , m_dataMapping(std::make_unique<DataMapping>(*this))
    , m_scalarMappingChooser(new ColorMappingChooser())
    , m_vectorMappingChooser(new GlyphMappingChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
    , m_rendererConfigWidget(new RendererConfigWidget())
    , m_canvasExporter(new CanvasExporterWidget(this))
    , m_loadWatchersMutex(std::make_unique<QMutex>())
{
    m_defaultPalette = qApp->palette();

    TextureManager::initialize();
    Loader::initialize();

    m_ui->setupUi(this);

    connect(m_ui->actionOpen, &QAction::triggered, [this] () { openFilesAsync(dialog_inputFileName()); });
    connect(m_ui->actionExportDataset, &QAction::triggered, this, &MainWindow::dialog_exportDataSet);
    connect(m_ui->actionAbout_Qt, &QAction::triggered, [this] () { QMessageBox::aboutQt(this); });
    connect(m_ui->actionApply_Digital_Elevation_Model, &QAction::triggered, this, &MainWindow::showDEMWidget);
    connect(m_ui->actionNew_Render_View, &QAction::triggered, 
        [this] () { m_dataMapping->openInRenderView({}); });
    connect(m_ui->actionExit, &QAction::triggered, [this] () { qApp->quit(); });


    m_dataBrowser = m_ui->centralwidget;
    m_dataBrowser->setDataMapping(m_dataMapping.get());

    SelectionHandler::instance().setSyncToggleMenu(m_ui->menuSynchronize_Selections);

    setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
    setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_scalarMappingChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_vectorMappingChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_rendererConfigWidget);
    tabifyDockWidget(m_renderConfigWidget, m_vectorMappingChooser);
    tabifyDockWidget(m_renderConfigWidget, m_rendererConfigWidget);
    tabbedDockWidgetToFront(m_renderConfigWidget);

    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_scalarMappingChooser, &ColorMappingChooser::setCurrentRenderView);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_vectorMappingChooser, &GlyphMappingChooser::setCurrentRenderView);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_renderConfigWidget, &RenderConfigWidget::setCurrentRenderView);

    connect(m_dataMapping.get(), &DataMapping::renderViewsChanged, m_rendererConfigWidget, &RendererConfigWidget::setRenderViews);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged,
        m_rendererConfigWidget, static_cast<void(RendererConfigWidget::*)(AbstractRenderView*)>(&RendererConfigWidget::setCurrentRenderView));

    connect(m_dataBrowser, &DataBrowser::selectedDataChanged,
        [this] (DataObject * selected) {
        m_renderConfigWidget->setSelectedData(selected);
        m_vectorMappingChooser->setSelectedData(selected);
    });

    connect(m_ui->actionSetup_Image_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::show);
    connect(m_ui->actionQuick_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshot);
    connect(m_ui->actionExport_To, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshotTo);
    connect(m_dataMapping.get(), &DataMapping::focusedRenderViewChanged, m_canvasExporter, &CanvasExporterWidget::setRenderView);

    connect(m_ui->actionObservation_Model_Residual_View, &QAction::triggered,
        [this] (bool) {
        auto view = DataMapping::instance().createRenderView<ResidualVerificationView>();
        DataMapping::instance().setFocusedView(view);
    });

    connect(m_ui->actionDark_Style, &QAction::triggered, this, &MainWindow::setDarkFusionStyle);

    //connect(m_ui->actionShow_Leak_Debugger, &QAction::triggered, m_debugLeaksView, &QWidget::show);
}

MainWindow::~MainWindow()
{
    while (true)
    {
        QFutureWatcher<void> * watcher;
        {
            QMutexLocker lock(m_loadWatchersMutex.get());
            if (m_loadWatchers.empty())
                break;

            watcher = m_loadWatchers.begin()->first.get();
        }
        watcher->waitForFinished();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    m_dataMapping.reset();
    // wait to close all views
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    m_ui.reset();

    TextureManager::release();

    //delete m_debugLeaksView;
}

QStringList MainWindow::dialog_inputFileName()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "", m_lastOpenFolder, Loader::fileFormatFilters());

    if (fileNames.isEmpty())
        return {};

    m_lastOpenFolder = QFileInfo(fileNames.first()).absolutePath();

    return fileNames;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent * event)
{
    assert(event->mimeData()->hasUrls());

    event->acceptProposedAction();

    QStringList fileNames;
    for (const QUrl & url : event->mimeData()->urls())
        fileNames << url.toLocalFile();

    openFilesAsync(fileNames);
}

void MainWindow::addRenderView(AbstractRenderView * renderView)
{
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, renderView->dockWidgetParent());

    connect(renderView, &AbstractRenderView::selectedDataChanged,
        [this] (AbstractRenderView * renderView, DataObject * selectedData) {
        m_dataMapping->setFocusedView(renderView);
        m_dataBrowser->setSelectedData(selectedData);
    });
}

void MainWindow::openFiles(const QStringList & fileNames)
{
    QString oldName = windowTitle();

    std::vector<std::unique_ptr<DataObject>> newData;

    for (QString fileName : fileNames)
    {
        qDebug() << " <" << fileName;

        if (!QFileInfo(fileName).exists())
        {
            qDebug() << "\t\t File does not exist!";
            continue;
        }

        auto dataObject = Loader::readFile(fileName);
        if (!dataObject)
        {
            QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
            setWindowTitle(oldName);
            continue;
        }

        newData.push_back(std::move(dataObject));
    }

    DataSetHandler::instance().takeData(std::move(newData));
}

void MainWindow::openFilesAsync(const QStringList & fileNames)
{
    auto watcher = std::make_unique<QFutureWatcher<void>>();
    auto watcherPtr = watcher.get();
    connect(watcherPtr, &QFutureWatcher<void>::finished, this, &MainWindow::handleAsyncLoadFinished);

    m_loadWatchersMutex->lock();
    m_loadWatchers.emplace(std::move(watcher), fileNames);
    m_loadWatchersMutex->unlock();

    watcherPtr->setFuture(
        QtConcurrent::run(this, &MainWindow::openFiles, fileNames));

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

    for (DataObject * data : toExport)
    {
        bool isSupported = Exporter::isExportSupported(data);
        if (!isSupported)
        {
            QMessageBox::warning(this, "Unsuppored Operation",
                "Exporting is currently not supported for the selected data type (" + data->dataTypeName() + ")\n"
                + "Data set: """ + data->name() + """");
            continue;
        }

        QString fileName = QFileDialog::getSaveFileName(this, "", m_lastExportFolder + "/" + data->name(),
            Exporter::formatFilter(data));

        if (fileName.isEmpty())
            continue;

        setWindowTitle("Exporting " + QFileInfo(fileName).baseName() + " ...");
        QApplication::processEvents();

        Exporter::exportData(data, fileName);

        m_lastExportFolder = QFileInfo(fileName).absolutePath();
    }

    setWindowTitle(oldName);
}

void MainWindow::showDEMWidget()
{
    DEMWidget * demWidget = new DEMWidget();
    demWidget->setAttribute(Qt::WA_DeleteOnClose);
    demWidget->setWindowModality(Qt::NonModal);
    demWidget->show();
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
        widgetzeug::enableDarkFusionStyle();
    else
    {
#ifdef _WINDOWS
        qApp->setStyle(QStyleFactory::create("windowsvista"));
#else
        qApp->setStyle(QStyleFactory::create("gtk"));
#endif
        qApp->setStyleSheet("");
        qApp->setPalette(m_defaultPalette);
    }
}

void MainWindow::updateWindowTitle()
{
    QMutexLocker lock(m_loadWatchersMutex.get());

    QString title = s_defaultAppTitle;

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
        assert(dynamic_cast<QFutureWatcher<void> *>(sender()));
        auto watcher = static_cast<QFutureWatcher<void> *>(sender());

        decltype(m_loadWatchers)::iterator toDeleteIt;
        for (toDeleteIt = m_loadWatchers.begin(); toDeleteIt != m_loadWatchers.end(); ++toDeleteIt)
        {
            if (toDeleteIt->first.get() == watcher)
                break;
        }
        assert(toDeleteIt != m_loadWatchers.end());
        m_loadWatchers.erase(toDeleteIt);
    }

    updateWindowTitle();
}
