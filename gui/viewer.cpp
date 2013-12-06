#include "viewer.h"
#include "ui_viewer.h"

// utility etc
#include <vtkSmartPointer.h>
#include <vtkTransform.h>


// sources
#include <vtkConeSource.h>
#include <vtkVectorText.h>
// filters
#include <vtkElevationFilter.h>
// mappers
#include <vtkPolyDataMapper.h>
// rendering
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
// interaction
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkBoxWidget.h>
// gui
#include <vtkQtTableView.h>
#include <vtkDataObjectToTable.h>

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
        vtkBoxWidget *widget = reinterpret_cast<vtkBoxWidget*>(caller);
        widget->GetTransform(t);
        widget->GetProp3D()->SetUserTransform(t);
        t->Delete();
    }
};

Viewer::Viewer()
: m_ui(new Ui_Viewer())
{
    m_ui->setupUi(this);

    m_tableView = vtkSmartPointer<vtkQtTableView>::New();
    m_ui->tableFrame->layout()->addWidget(m_tableView->GetWidget());

    connect(m_ui->actionExit, SIGNAL(triggered()), SLOT(slotExit()));

    /** create cone with mapper and actor */
    vtkSmartPointer<vtkConeSource> cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetHeight(3.0);
    cone->SetRadius(1.0);
    cone->SetResolution(10);

    vtkSmartPointer<vtkPolyDataMapper> coneMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    coneMapper->SetInputConnection(cone->GetOutputPort());

    vtkSmartPointer<vtkActor> cone1Actor = vtkSmartPointer<vtkActor>::New();
    cone1Actor->SetMapper(coneMapper);
    vtkSmartPointer<vtkActor> cone2Actor = vtkSmartPointer<vtkActor>::New();
    cone2Actor->SetMapper(coneMapper);
    cone2Actor->SetOrientation(0, 0, 180);

    /** create text with filter, mapper and actor */
    vtkSmartPointer<vtkVectorText> text = vtkSmartPointer<vtkVectorText>::New();
    text->SetText("vtk-qt-testing");

    vtkSmartPointer<vtkElevationFilter> elevation = vtkSmartPointer<vtkElevationFilter>::New();
    elevation->SetInputConnection(text->GetOutputPort());
    elevation->SetLowPoint(0, 0, 0);
    elevation->SetHighPoint(10, 0, 0);

    vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    textMapper->ImmediateModeRenderingOn();
    textMapper->SetInputConnection(elevation->GetOutputPort());

    vtkSmartPointer<vtkActor> textActor = vtkSmartPointer<vtkActor>::New();
    textActor->SetMapper(textMapper);
    textActor->SetPosition(-6.0, 3.0, 0.0);


    /** create renderer and add actors */
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(cone1Actor);
    renderer->AddActor(cone2Actor);
    renderer->AddActor(textActor);

    m_ui->qvtkWidget->GetRenderWindow()->AddRenderer(renderer);

    /** pass infos to qt gui */
    vtkSmartPointer<vtkDataObjectToTable> toTable = vtkSmartPointer<vtkDataObjectToTable>::New();
    toTable->SetInputConnection(elevation->GetOutputPort());
    toTable->SetFieldType(vtkDataObjectToTable::POINT_DATA);

    m_tableView->SetRepresentationFromInputConnection(toTable->GetOutputPort());
    m_tableView->Update();

    /** setup interaction */
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> interactStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interactor->SetInteractorStyle(interactStyle);
    m_interactor->SetRenderWindow(m_ui->qvtkWidget->GetRenderWindow());
    vtkSmartPointer<vtkBoxWidget> boxWidget = vtkSmartPointer<vtkBoxWidget>::New();
    boxWidget->SetInteractor(m_interactor);
    boxWidget->SetPlaceFactor(1.25);

    boxWidget->SetProp3D(cone1Actor);
    boxWidget->PlaceWidget();
    vtkSmartPointer<TransformCallback> callback = vtkSmartPointer<TransformCallback>::New();
    boxWidget->AddObserver(vtkCommand::InteractionEvent, callback);

    boxWidget->On();

    m_interactor->Initialize();
    //m_interactor->Start(); // this starts the integrated event loop

}

Viewer::~Viewer()
{

}

void Viewer::slotExit()
{
    close();
}
