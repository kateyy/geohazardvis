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
#include "SelectionHandler.h"
#include "widgets/RenderWidget.h"
#include "widgets/DataChooser.h"
#include "widgets/RenderConfigWidget.h"


using namespace std;

MainWindow::MainWindow()
    : QMainWindow()
    , m_ui(new Ui_MainWindow())
    , m_dataMapping(new DataMapping(*this))
    , m_dataChooser(new DataChooser())
    , m_renderConfigWidget(new RenderConfigWidget())
{
    m_ui->setupUi(this);

    m_dataBrowser = m_ui->centralwidget;
    m_dataBrowser->setDataMapping(m_dataMapping);

    SelectionHandler::instance().setSyncToggleMenu(m_ui->menuSynchronize_Selections);

    setCorner(Qt::Corner::TopLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::BottomLeftCorner, Qt::DockWidgetArea::LeftDockWidgetArea);
    setCorner(Qt::Corner::TopRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);
    setCorner(Qt::Corner::BottomRightCorner, Qt::DockWidgetArea::RightDockWidgetArea);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_dataChooser);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_renderConfigWidget);







}

MainWindow::~MainWindow()
{
    delete m_ui;
    delete m_dataMapping;
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
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, renderWidget);

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

    m_dataBrowser->addDataObject(dataObject);

    m_dataMapping->addDataObjects({ dataObject });

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

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
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

