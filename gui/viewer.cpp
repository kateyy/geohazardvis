#include "viewer.h"
#include "ui_viewer.h"

#include <cassert>

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkProperty.h>

// mappers
#include <vtkPolyDataMapper.h>
// actors
#include <vtkActor.h>
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
    /** setup interaction */
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
    Input input;

    input.inputDataMapper = Loader::loadFileTriangulated("data/Tcoord_topo.txt", 4, 1);
    input.setColor(0, 0, 1);
    m_inputs.push_back(input);

    input.inputDataMapper = Loader::loadFileTriangulated("data/Tvert.txt", 6, 0);
    input.setColor(1, 0, 0);
    m_inputs.push_back(input);

    m_mainRenderer->AddActor(m_inputs.front().createActor());
    m_infoRenderer->AddActor(m_inputs.back().createActor());
}

void Viewer::ShowInfo(const QStringList & info)
{
    m_ui->infoBox->clear();

    m_ui->infoBox->addItems(info);
}

void Viewer::Input::setColor(double r, double g, double b)
{
    color[0] = r; color[1] = g; color[2] = b;
}

vtkSmartPointer<vtkActor> Viewer::Input::createActor()
{
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(inputDataMapper);
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->SetColor(color);

    return actor;
}
