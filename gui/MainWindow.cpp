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

#include "SelectionHandler.h"
#include "QVtkTableModel.h"
#include "widgets/RenderView.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"
#include "widgets/TableWidget.h"


using namespace std;

MainWindow::MainWindow()
: QMainWindow()
, m_ui(new Ui_MainWindow())
, m_renderView(nullptr)
, m_tableWidget(new TableWidget())
, m_dataChooser(new DataChooser())
, m_renderConfigWidget(new RenderConfigWidget())
{
    m_ui->setupUi(this);

    m_selectionHandler = make_shared<SelectionHandler>();

    m_renderView = new RenderView(*m_dataChooser, *m_renderConfigWidget, m_selectionHandler);

    m_selectionHandler->setQtTableView(m_tableWidget->tableView(), m_tableWidget->model());
    m_selectionHandler->setVtkInteractionStyle(m_renderView->interactStyle());

    setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
    setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_renderView);
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, m_tableWidget);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_dataChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);

    connect(m_dataChooser, &DataChooser::selectionChanged, m_renderView, &RenderView::updateScalarsForColorMaping);
    connect(m_renderConfigWidget, &RenderConfigWidget::gradientSelectionChanged, m_renderView, &RenderView::updateGradientForColorMapping);
    connect(m_renderConfigWidget, &RenderConfigWidget::renderPropertyChanged, m_renderView, &RenderView::render);
}

MainWindow::~MainWindow()
{
    delete m_ui;
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

    setWindowTitle(QString::fromStdString(input->name) + " (loading to gpu)");
    QApplication::processEvents();

    m_inputs = { input };

    switch (input->type) {
    case ModelType::triangles:
        m_renderView->show3DInput(std::dynamic_pointer_cast<PolyDataInput>(input));
        m_tableWidget->showData(static_cast<PolyDataInput&>(*input).data());
        break;
    case ModelType::grid2d:
        m_renderView->showGridInput(std::dynamic_pointer_cast<GridDataInput>(input));
        m_tableWidget->showData(static_cast<GridDataInput&>(*input).data());
        break;
    default:
        QMessageBox::critical(this, "File error", "Could not open the selected input file. (unsupported format)");
        return;
    }

    setWindowTitle(QString::fromStdString(input->name));
    QApplication::processEvents();
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
