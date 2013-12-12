#pragma once

#include <QMainWindow>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkQtTableView;
class vtkRenderer;
class vtkRenderWindowInteractor;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();
    virtual ~Viewer();

private:
    Ui_Viewer *m_ui;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderer> m_infoRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;

    void setupRenderer();
    void setupInteraction();
    void loadInputs();

    void addLabels();

    vtkSmartPointer<vtkQtTableView> m_tableView;
};
