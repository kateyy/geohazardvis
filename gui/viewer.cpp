#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkProperty.h>

// inputs
#include <vtkPolyDataAlgorithm.h>
// filters
#include <vtkElevationFilter.h>
// mappers
#include <vtkPolyDataMapper.h>
// actors
#include <vtkActor.h>
#include <vtkAxisActor.h>
// rendering
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
// interaction
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include "pickinginteractionstyle.h"
#include <vtkBoxWidget.h>
#include <vtkPointPicker.h>
// gui/qt
#include <QVTKWidget.h>
#include <QTreeView>

#include "core/loader.h"

class TransformCallback : public vtkCommand
{
public:
    static TransformCallback *New()
    {
        return new TransformCallback;
    }
    virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
        vtkTransform *t = vtkTransform::New();
        vtkBoxWidget *widget = dynamic_cast<vtkBoxWidget*>(caller);
        assert(widget);
        widget->GetTransform(t);
        widget->GetProp3D()->SetUserTransform(t);
        t->Delete();
    }
};

Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);

    setupRenderer();
    setupInteraction();

    loadInputs();

    m_mainRenderer->ResetCamera();
}

void Viewer::setupRenderer()
{
    m_mainRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_mainRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkMain->GetRenderWindow()->AddRenderer(m_mainRenderer);

    m_infoRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_infoRenderer->SetBackground(1, 1, 1);
    m_ui->qvtkInfo->GetRenderWindow()->AddRenderer(m_infoRenderer);
}

void Viewer::setupInteraction()
{
    vtkSmartPointer<PickingInteractionStyle> interactStyle = vtkSmartPointer<PickingInteractionStyle>::New();
    interactStyle->SetDefaultRenderer(m_mainRenderer);
    interactStyle->setViewer(*this);
    m_mainInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_mainInteractor->SetInteractorStyle(interactStyle);
    m_mainInteractor->SetRenderWindow(m_ui->qvtkMain->GetRenderWindow());

    m_mainInteractor->Initialize();
}

void Viewer::loadInputs()
{
    m_inputs.push_back(Loader::loadFileTriangulated("data/Tcoord_topo.txt", 4, 1));
    m_inputs.push_back(Loader::loadFileAsPoints("data/Tvert.txt", 6, 0));

    std::shared_ptr<Input> input1 = m_inputs.front();
    std::shared_ptr<Input> input2 = m_inputs.back();

    // use the elevation for colorized visualization
    vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
    elevation->SetInputConnection(input1->algorithm->GetOutputPort());
    elevation->SetLowPoint(0, 0, 4);
    elevation->SetHighPoint(0, 0, 0);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(elevation->GetOutputPort());

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    m_mainRenderer->AddActor(actor);
    vtkSmartPointer<vtkActor> actor2 = input2->createActor();
    actor2->GetProperty()->SetColor(0, 1, 0);
    m_infoRenderer->AddActor(actor2);

    double bounds[6];
    actor->GetBounds(bounds);
    for (int i = 0; i < 6; ++i)
        std::cout << bounds[i] << " ";
    std::cout << std::endl;
}

void Viewer::setupAxis()
{
    vtkSmartPointer<vtkAxisActor> axisActor = vtkSmartPointer<vtkAxisActor>::New();


}

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);
}
