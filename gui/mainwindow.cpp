#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cassert>

#include <QFileDialog>
#include <QMessageBox>

#include "inputviewer.h"

MainWindow::MainWindow()
: QMainWindow()
, m_ui(new Ui_MainWindow())
{
    m_ui->setupUi(this);

    connect(m_ui->tabWidget, &TabWidget::tabPopOutClicked, this, &MainWindow::untabifyViewer);
    connect(m_ui->tabWidget, &TabWidget::tabCloseRequested, this, &MainWindow::closeViewer);
    
    createEmptyViewerTabbed();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

InputViewer * MainWindow::createEmptyViewerTabbed()
{
    // use the current viewer if it's empty
    InputViewer * currentViewer = dynamic_cast<InputViewer*>(m_ui->tabWidget->currentWidget());
    if (currentViewer && currentViewer->isEmpty())
        return currentViewer;


    InputViewer * emptyViewer = new InputViewer();

    int newIndex = m_ui->tabWidget->addTab(emptyViewer, emptyViewer->windowTitle());
    m_ui->tabWidget->setCurrentIndex(newIndex);

    connect(emptyViewer, &InputViewer::windowTitleChanged, this, &MainWindow::viewerTitleChanged);
    connect(emptyViewer, &InputViewer::dockingRequested, this, &MainWindow::tabifyViewer);

    return emptyViewer;
}

void MainWindow::closeViewer(int tabIndex)
{
    InputViewer * viewer = dynamic_cast<InputViewer*>(m_ui->tabWidget->widget(tabIndex));

    // can't close the last viewer
    if (viewer->isEmpty() && m_ui->tabWidget->count() == 1)
        return;

    m_ui->tabWidget->removeTab(tabIndex);

    viewer->deleteLater();

    checkForEmptyViewer();
}

void MainWindow::checkForEmptyViewer()
{
    if (m_ui->tabWidget->count() == 0)
    {
        createEmptyViewerTabbed();
        return;
    }

    for (int index = 0; index < m_ui->tabWidget->count(); )
    {
        InputViewer * viewer = dynamic_cast<InputViewer*>(m_ui->tabWidget->widget(index));
        assert(viewer);

        if (!viewer->isEmpty()) {
            ++index;
            continue;
        }

        m_ui->tabWidget->removeTab(index);
        viewer->deleteLater();
    }
}

void MainWindow::tabifyViewer()
{
    InputViewer * windowedViewer = dynamic_cast<InputViewer*>(sender());
    assert(windowedViewer);

    windowedViewer->setWindowFlags(Qt::Widget);
    m_ui->tabWidget->insertTab(m_ui->tabWidget->currentIndex(), windowedViewer, windowedViewer->windowTitle());
    m_ui->tabWidget->setCurrentWidget(windowedViewer);

    checkForEmptyViewer();
}

void MainWindow::untabifyViewer(int tabIndex)
{
    assert(tabIndex >= 0 && tabIndex < m_ui->tabWidget->count());

    InputViewer * tabbedViewer = dynamic_cast<InputViewer*>(m_ui->tabWidget->widget(tabIndex));
    assert(tabbedViewer);

    m_ui->tabWidget->removeTab(tabIndex);

    tabbedViewer->setWindowFlags(Qt::Window);
    tabbedViewer->show();

    // fix position: relative to screen, not main window
    tabbedViewer->move(QPoint(geometry().x(), geometry().y()));

    checkForEmptyViewer();

    QApplication::processEvents();

    tabbedViewer->activateWindow();
}

void MainWindow::viewerTitleChanged(const QString & title)
{
    QWidget * viewer = dynamic_cast<QWidget*>(sender());
    assert(viewer);

    int index = m_ui->tabWidget->indexOf(viewer);
    assert(index >= 0);

    m_ui->tabWidget->setTabText(index, title);
}

QString MainWindow::dialog_inputFileName()
{
    QString fileName = QFileDialog::getOpenFileName(this, "", m_lastOpenFolder, "Text files (*.txt)");

    if (fileName.isEmpty())
        return QString();

    m_lastOpenFolder = QFileInfo(fileName).absolutePath();

    return fileName;
}

void MainWindow::on_actionOpen_currentTab_triggered()
{
    InputViewer * currentViewer = dynamic_cast<InputViewer*>(m_ui->tabWidget->currentWidget());
    if (currentViewer == nullptr) {
        QMessageBox::warning(this, "Warning", "Cannot open a file in the current tab. The widget is not an InputViewer.");
        return;
    }

    QString fileName = dialog_inputFileName();

    if (fileName.isEmpty())
        return;

    emit currentViewer->openFile(fileName);
}

void MainWindow::on_actionOpen_newTab_triggered()
{
    QString fileName = dialog_inputFileName();

    if (fileName.isEmpty())
        return;

    InputViewer * newViewer = createEmptyViewerTabbed();

    emit newViewer->openFile(fileName);

    checkForEmptyViewer();
}
