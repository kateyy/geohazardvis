#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

#include <vtkPolyData.h>

#include <core/vtkhelper.h>
#include <core/Loader.h>
#include <core/Input.h>
#include <core/QVtkTableModel.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/data_objects/ImageDataObject.h>

#include "DataMapping.h"
#include "LoadedFilesTableModel.h"
#include "widgets/RenderWidget.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;

MainWindow::MainWindow()
    : QMainWindow()
    , m_ui(new Ui_MainWindow())
    , m_loadedFilesModel(new LoadedFilesTableModel())
    , m_dataMapping(new DataMapping(*this))
    , m_dataChooser(new DataChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
{
    m_ui->setupUi(this);

    m_ui->loadedFiles->setModel(m_loadedFilesModel);

    setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
    setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_dataChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);

    QAction * a_inputTable = new QAction("show table", this);
    connect(a_inputTable, &QAction::triggered, this, &MainWindow::openTable);
    m_ui->loadedFiles->addAction(a_inputTable);

    QAction * a_openRenderView = new QAction("open render view", this);
    connect(a_openRenderView, &QAction::triggered, this, &MainWindow::openRenderView);
    m_ui->loadedFiles->addAction(a_openRenderView);

    m_addToRendererAction = new QAction("add to render view", this);
    m_addToRendererAction->setEnabled(false);
    m_ui->loadedFiles->addAction(m_addToRendererAction);

    connect(m_dataMapping, &DataMapping::renderViewsChanged, this, &MainWindow::updateRenderViewActions);
}

MainWindow::~MainWindow()
{
    delete m_dataMapping;

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

    for (const QUrl & url : event->mimeData()->urls())
        openFile(url.toLocalFile());

    event->acceptProposedAction();
}

RenderWidget * MainWindow::addRenderWidget(int index)
{
    RenderWidget * renderWidget = new RenderWidget(index, *m_dataChooser, *m_renderConfigWidget);
    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, renderWidget);

    return renderWidget;
}

void MainWindow::openFile(QString fileName)
{
    qDebug() << " <" << fileName;

    QApplication::processEvents();

    QString oldName = windowTitle();
    setWindowTitle(fileName + " (loading file)");
    QApplication::processEvents();

    shared_ptr<Input> input = Loader::readFile(fileName.toStdString());
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        setWindowTitle(oldName);
        return;
    }

    DataObject * dataObject = nullptr;

    switch (input->type)
    {
    case ModelType::triangles:
        dataObject = new PolyDataObject(std::dynamic_pointer_cast<PolyDataInput>(input));
        break;
    case ModelType::grid2d:
        dataObject = new ImageDataObject(std::dynamic_pointer_cast<GridDataInput>(input));
        break;
    }

    m_dataObjects << dataObject;

    m_loadedFilesModel->addDataObject(dataObject);

    m_dataMapping->addDataObject(dataObject);

    setWindowTitle(oldName);
}

void MainWindow::openFiles(QStringList fileNames)
{
    for (const QString & fileName : fileNames)
        openFile(fileName);
}

void MainWindow::on_actionOpen_triggered()
{
    openFiles(dialog_inputFileName());
}

void MainWindow::updateRenderViewActions(QList<RenderWidget*> widgets)
{
    QMenu * menu = new QMenu(this);
    
    for (RenderWidget * widget : widgets)
    {
        QAction * a = new QAction(widget->windowTitle(), this);
        a->setData(QVariant(widget->index()));
        connect(a, &QAction::triggered, this, &MainWindow::addToRenderView);
        menu->addAction(a);
    }

    m_addToRendererAction->setMenu(menu);
    m_addToRendererAction->setEnabled(!menu->isEmpty());
}

DataObject * MainWindow::selectedDataObject()
{
    QModelIndexList selection = m_ui->loadedFiles->selectionModel()->selectedRows();
    if (selection.isEmpty())
        return nullptr;

    std::string selectedName = m_loadedFilesModel->data(selection.first()).toString().toStdString();

    for (auto dataObject : m_dataObjects)
    {
        if (selectedName == dataObject->input()->name)
            return dataObject;
    }
    assert(false);
    return nullptr;
}

void MainWindow::openTable()
{
    DataObject * dataObject = selectedDataObject();
    if (dataObject)
        m_dataMapping->openInTable(dataObject);
}

void MainWindow::openRenderView()
{
    DataObject * dataObject = selectedDataObject();
    if (dataObject)
        m_dataMapping->openInRenderView(dataObject);
}

void MainWindow::addToRenderView()
{
    QAction * action = dynamic_cast<QAction*>(sender());
    assert(action);
    bool ok = false;
    int index = action->data().toInt(&ok);
    assert(ok);

    DataObject * dataObject = selectedDataObject();
    if (dataObject)
        m_dataMapping->addToRenderView(dataObject, index);
}
