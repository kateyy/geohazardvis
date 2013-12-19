#pragma once

#include <QMainWindow>

#include <vtkSmartPointer.h>

class Ui_Viewer;
class vtkQtTableView;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkPolyDataMapper;

class vtkPointPicker;

class Viewer : public QMainWindow
{
    Q_OBJECT

public:
    Viewer();

public slots:
    void ShowInfo(QString info);

private:
    Ui_Viewer *m_ui;

    vtkSmartPointer<vtkRenderer> m_mainRenderer;
    vtkSmartPointer<vtkRenderer> m_infoRenderer;
    vtkSmartPointer<vtkRenderWindowInteractor> m_mainInteractor;
    vtkSmartPointer<vtkRenderWindowInteractor> m_infoInteractor;

    vtkSmartPointer<vtkPolyDataMapper> m_volcanoMapper;
    vtkSmartPointer<vtkPolyDataMapper> m_volcanoCoreMapper;

    void setupRenderer();
    void setupInteraction();
    void loadInputs();

    void addInfos();

    void addLabels();
};
