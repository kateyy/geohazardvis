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

#include "DataMapping.h"
#include "SelectionHandler.h"
#include "data_view/RenderView.h"
#include "widgets/ScalarMappingChooser.h"
#include "widgets/RenderConfigWidget.h"


MainWindow::MainWindow()
    : QMainWindow()
    , m_ui(new Ui_MainWindow())
    , m_dataMapping(new DataMapping(*this))
    , m_scalarMappingChooser(new ScalarMappingChooser())
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

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_scalarMappingChooser);
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

RenderView * MainWindow::addRenderView(int index)
{
    RenderView * renderView = new RenderView(index, *m_scalarMappingChooser, *m_renderConfigWidget);
    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, renderView);

    return renderView;
}

void MainWindow::openFile(QString fileName)
{
    qDebug() << " <" << fileName;

    QApplication::processEvents();

    QString oldName = windowTitle();
    setWindowTitle(fileName + " (loading file)");
    QApplication::processEvents();

    DataObject * dataObject = Loader::readFile(fileName);
    if (!dataObject)
    {
        QMessageBox::critical(this, "File error", "Could not open the selected input file (unsupported format).");
        setWindowTitle(oldName);
        return;
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
