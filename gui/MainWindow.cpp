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

#include <core/utility/vtkhelper.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/io/Exporter.h>
#include <core/io/Loader.h>
#include <core/rendered_data/RenderedData.h>
#include <core/TextureManager.h>

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
    , m_ui(new Ui_MainWindow())
    , m_dataMapping(new DataMapping(*this))
    , m_scalarMappingChooser(new ColorMappingChooser())
    , m_vectorMappingChooser(new GlyphMappingChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
    , m_rendererConfigWidget(new RendererConfigWidget())
    , m_canvasExporter(new CanvasExporterWidget(this))
    , m_loadWatchersMutex(new QMutex())
{
    m_defaultPalette = qApp->palette();

    TextureManager::initialize();
    Loader::initialize();

    m_ui->setupUi(this);

    m_dataBrowser = m_ui->centralwidget;
    m_dataBrowser->setDataMapping(m_dataMapping);

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

    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_scalarMappingChooser, &ColorMappingChooser::setCurrentRenderView);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_vectorMappingChooser, &GlyphMappingChooser::setCurrentRenderView);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_renderConfigWidget, &RenderConfigWidget::setCurrentRenderView);

    connect(m_dataMapping, &DataMapping::renderViewsChanged, m_rendererConfigWidget, &RendererConfigWidget::setRenderViews);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged,
        m_rendererConfigWidget, static_cast<void(RendererConfigWidget::*)(AbstractRenderView*)>(&RendererConfigWidget::setCurrentRenderView));

    connect(m_dataBrowser, &DataBrowser::selectedDataChanged,
        [this] (DataObject * selected) {
        m_renderConfigWidget->setSelectedData(selected);
        m_vectorMappingChooser->setSelectedData(selected);
    });

    connect(m_ui->actionSetup_Image_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::show);
    connect(m_ui->actionQuick_Export, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshot);
    connect(m_ui->actionExport_To, &QAction::triggered, m_canvasExporter, &CanvasExporterWidget::captureScreenshotTo);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_canvasExporter, &CanvasExporterWidget::setRenderView);

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
            QMutexLocker lock(m_loadWatchersMutex);
            if (m_loadWatchers.isEmpty())
                break;

            watcher = m_loadWatchers.begin().key();
        }
        watcher->waitForFinished();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    delete m_loadWatchersMutex;

    delete m_dataMapping;
    // wait to close all views
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    delete m_ui;

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

    QList<DataObject *> newData;

    for (QString fileName : fileNames)
    {
        qDebug() << " <" << fileName;

        if (!QFileInfo(fileName).exists())
        {
            qDebug() << "\t\t File does not exist!";
            continue;
        }

        DataObject * dataObject = Loader::readFile(fileName);
        if (!dataObject)
        {
            QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
            setWindowTitle(oldName);
            continue;
        }

        newData << dataObject;
    }

    DataSetHandler::instance().addData(newData);
}

void MainWindow::openFilesAsync(const QStringList & fileNames)
{
    auto watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, &MainWindow::handleAsyncLoadFinished);

    m_loadWatchersMutex->lock();
    m_loadWatchers.insert(watcher, fileNames);
    m_loadWatchersMutex->unlock();

    watcher->setFuture(
        QtConcurrent::run(this, &MainWindow::openFiles, fileNames));

    updateWindowTitle();
    QApplication::processEvents();
}

void MainWindow::on_actionOpen_triggered()
{
    openFilesAsync(dialog_inputFileName());
}

void MainWindow::on_actionExportDataset_triggered()
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

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionNew_Render_View_triggered()
{
    m_dataMapping->openInRenderView({});
}

void MainWindow::on_actionApply_Digital_Elevation_Model_triggered()
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
    QMutexLocker lock(m_loadWatchersMutex);

    QString title = s_defaultAppTitle;

    if (!m_loadWatchers.isEmpty())
    {
        title += " -  Loading Files: ";

        for (auto & fileNames : m_loadWatchers.values())
        {
            for (auto & fileName : fileNames)
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
        QMutexLocker lock(m_loadWatchersMutex);
        assert(dynamic_cast<QFutureWatcher<void> *>(sender()));
        auto watcher = static_cast<QFutureWatcher<void> *>(sender());
        m_loadWatchers.remove(watcher);
    }

    updateWindowTitle();
}
