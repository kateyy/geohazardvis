#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

#include "inputviewer.h"

MainWindow::MainWindow()
: QMainWindow()
, m_ui(new Ui_MainWindow())
{
    m_ui->setupUi(this);
    
    InputViewer * emptyViewer = new InputViewer();

    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, emptyViewer);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::on_actionOpen_triggered()
{
    static QString lastFolder;
    QString filename = QFileDialog::getOpenFileName(this, "", lastFolder, "Text files (*.txt)");
    if (filename.isEmpty())
        return;

    lastFolder = QFileInfo(filename).absolutePath();

    InputViewer * viewer = new InputViewer();

    addDockWidget(Qt::DockWidgetArea::TopDockWidgetArea, viewer);

    QApplication::processEvents();

    emit viewer->openFile(filename);
}
