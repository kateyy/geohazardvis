#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QDebug>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QMessageBox>
#include <QMimeData>

#include <core/vtkhelper.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/ImageDataObject.h>
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


MainWindow::MainWindow()
    : QMainWindow()
    , m_ui(new Ui_MainWindow())
    , m_dataMapping(new DataMapping(*this))
    , m_scalarMappingChooser(new ColorMappingChooser())
    , m_vectorMappingChooser(new GlyphMappingChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
    , m_rendererConfigWidget(new RendererConfigWidget())
    , m_canvasExporter(new CanvasExporterWidget(this))
{
    TextureManager::initialize();

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

    connect(m_ui->actionSetup_Image_Export, &QAction::triggered,
        [this] (bool) { m_canvasExporter->show(); });
    connect(m_ui->actionQuick_Export, &QAction::triggered, 
        [this] (bool) { m_canvasExporter->captureScreenshot(); });
    connect(m_ui->actionExport_To, &QAction::triggered,
        [this] (bool) { m_canvasExporter->captureScreenshotTo(); });
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_canvasExporter, &CanvasExporterWidget::setRenderView);

    connect(m_ui->actionResidual_Test, &QAction::triggered, [this] (bool) {
        ImageDataObject * observation = nullptr;
        for (auto data : DataSetHandler::instance().dataSets())
        {
            observation = dynamic_cast<ImageDataObject *>(data);
            if (observation)
                break;
        }
        if (!observation)
            return;

        auto view = DataMapping::instance().createRenderView<ResidualVerificationView>();
        view->setObservationData(observation);
    });
}

MainWindow::~MainWindow()
{
    delete m_dataMapping;
    // wait to close all views
    QApplication::processEvents();
    delete m_ui;

    TextureManager::release();
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

    openFiles(fileNames);
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

        setWindowTitle(fileName + " (loading file)");
        QApplication::processEvents();

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

    setWindowTitle(oldName);
}

void MainWindow::on_actionOpen_triggered()
{
    openFiles(dialog_inputFileName());
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
