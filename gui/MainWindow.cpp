#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

#include <core/vtkhelper.h>
#include <core/Loader.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>

#include <gui/DataMapping.h>
#include <gui/SelectionHandler.h>
#include <gui/data_view/RenderView.h>
#include <gui/widgets/CanvasExporterWidget.h>
#include <gui/widgets/ScalarMappingChooser.h>
#include <gui/widgets/VectorMappingChooser.h>
#include <gui/widgets/RenderConfigWidget.h>
#include <gui/widgets/RendererConfigWidget.h>


MainWindow::MainWindow()
    : QMainWindow()
    , m_ui(new Ui_MainWindow())
    , m_dataMapping(new DataMapping(*this))
    , m_scalarMappingChooser(new ScalarMappingChooser())
    , m_vectorMappingChooser(new VectorMappingChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
    , m_rendererConfigWidget(new RendererConfigWidget())
    , m_canvasExporter(new CanvasExporterWidget(this))
{
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

    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_scalarMappingChooser, &ScalarMappingChooser::setCurrentRenderView);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_vectorMappingChooser, &VectorMappingChooser::setCurrentRenderView);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_renderConfigWidget, &RenderConfigWidget::setCurrentRenderView);

    connect(m_dataMapping, &DataMapping::renderViewsChanged, m_rendererConfigWidget, &RendererConfigWidget::setRenderViews);
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged,
        m_rendererConfigWidget, static_cast<void(RendererConfigWidget::*)(RenderView*)>(&RendererConfigWidget::setCurrentRenderView));

    connect(m_dataBrowser, &DataBrowser::selectedDataChanged,
        [this] (DataObject * selected) {
        m_renderConfigWidget->setSelectedData(selected);
        m_vectorMappingChooser->setSelectedData(selected);
    });

    connect(m_ui->action_Quick_Export_with_Current_Settings, &QAction::triggered, 
        [this] (bool) { m_canvasExporter->captureScreenshot(); });
    connect(m_dataMapping, &DataMapping::focusedRenderViewChanged, m_canvasExporter, &CanvasExporterWidget::setRenderView);
    connect(m_ui->action_Export_Rendered_Image, &QAction::triggered,
        [this] (bool) { m_canvasExporter->exec(); });
}

MainWindow::~MainWindow()
{
    delete m_dataMapping;
    // wait to close all views
    QApplication::processEvents();
    delete m_ui;
}

QStringList MainWindow::dialog_inputFileName()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, "", m_lastOpenFolder, "Text files (*.txt)");

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

void MainWindow::addRenderView(RenderView * renderView)
{
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, renderView);

    connect(renderView, &RenderView::selectedDataChanged, 
        [this] (RenderView * renderView, DataObject * selectedData) {
        m_dataMapping->setFocusedView(renderView);
        m_dataBrowser->setSelectedData(selectedData);
    });
}

void MainWindow::openFiles(QStringList fileNames)
{
    QString oldName = windowTitle();

    QList<DataObject *> newData;

    for (QString fileName : fileNames)
    {
        qDebug() << " <" << fileName;

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

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionNew_Render_View_triggered()
{
    m_dataMapping->openInRenderView({});
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
