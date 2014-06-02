#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cassert>

#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDebug>

#include <vtkPolyData.h>

#include "core/vtkhelper.h"
#include "core/Loader.h"
#include "core/Input.h"

#include "InputRepresentation.h"
#include "DataMapping.h"
#include "SelectionHandler.h"
#include "QVtkTableModel.h"
#include "widgets/RenderWidget.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"
#include "widgets/TableWidget.h"


using namespace std;

MainWindow::MainWindow()
: QMainWindow()
, m_ui(new Ui_MainWindow())
, m_dataMapping(new DataMapping(*this))
, m_dataChooser(new DataChooser())
, m_renderConfigWidget(new RenderConfigWidget())
{
    m_ui->setupUi(this);

    m_selectionHandler = make_shared<SelectionHandler>();

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
    delete m_ui;
    delete m_dataMapping;
}

QString MainWindow::dialog_inputFileName()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", m_lastOpenFolder, "Text files (*.txt)");

    if (fileName.isEmpty())
        return QString();

    m_lastOpenFolder = QFileInfo(fileName).absolutePath();

    return fileName;
}

void MainWindow::dragEnterEvent(QDragEnterEvent * event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent * event)
{
    assert(event->mimeData()->hasUrls());
    QString filename = event->mimeData()->urls().first().toLocalFile();
    qDebug() << filename;

    emit openFile(filename);

    event->acceptProposedAction();
}

RenderWidget * MainWindow::addRenderWidget(int index)
{
    RenderWidget * renderWidget = new RenderWidget(index, *m_dataChooser, *m_renderConfigWidget, m_selectionHandler);
    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, renderWidget);

    connect(m_dataChooser, &DataChooser::selectionChanged, renderWidget, &RenderWidget::updateScalarsForColorMaping);
    connect(m_renderConfigWidget, &RenderConfigWidget::gradientSelectionChanged, renderWidget, &RenderWidget::updateGradientForColorMapping);
    connect(m_renderConfigWidget, &RenderConfigWidget::renderPropertyChanged, renderWidget, &RenderWidget::render);

    return renderWidget;
}

void MainWindow::openFile(QString filename)
{
    QApplication::processEvents();

    QString oldName = windowTitle();
    setWindowTitle(filename + " (loading file)");
    QApplication::processEvents();

    shared_ptr<Input> input = Loader::readFile(filename.toStdString());
    if (!input) {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        setWindowTitle(oldName);
        return;
    }

    std::shared_ptr<InputRepresentation> representation = std::make_shared<InputRepresentation>(input);

    m_inputs << representation;

    m_ui->loadedFiles->addItem(QString::fromStdString(representation->input()->name));

    m_dataMapping->addInputRepresenation(representation);

    setWindowTitle(oldName);
}

void MainWindow::on_actionOpen_currentTab_triggered()
{
    on_actionOpen_newTab_triggered();
}

void MainWindow::on_actionOpen_newTab_triggered()
{
    QString fileName = dialog_inputFileName();

    if (fileName.isEmpty())
        return;

    openFile(fileName);
}

void MainWindow::updateRenderViewActions(QList<RenderWidget*> widgets)
{
    QMenu * menu = new QMenu(this);
    
    for (RenderWidget * widget : widgets)
    {
        QAction * a = new QAction(widget->windowTitle(), this);
        menu->addAction(a);
    }

    m_addToRendererAction->setMenu(menu);
    m_addToRendererAction->setEnabled(!menu->isEmpty());
}

std::shared_ptr<InputRepresentation> MainWindow::selectedInput()
{
    QListWidgetItem * selection = m_ui->loadedFiles->currentItem();
    if (selection == nullptr)
        return nullptr;

    for (auto input : m_inputs)
    {
        if (selection->text().toStdString() == input->input()->name)
        {
            return input;
        }
    }
}

void MainWindow::openTable()
{
    auto input = selectedInput();
    if (input)
        m_dataMapping->openInTable(input);
}

void MainWindow::openRenderView()
{
    auto input = selectedInput();
    if (input)
        m_dataMapping->openInRenderView(input);
}

void MainWindow::addToRenderView()
{
    auto input = selectedInput();
    if (input)
        m_dataMapping->addToRenderView(input, 0);
}
