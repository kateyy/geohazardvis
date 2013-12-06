#pragma once

#include <QMainWindow>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkQtTableView;
class vtkRenderWindowInteractor;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();
    virtual ~Viewer();

public slots:
    virtual void slotExit();

private:
    Ui_Viewer *m_ui;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;

    vtkSmartPointer<vtkQtTableView> m_tableView;
};
