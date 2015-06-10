#include "InteractorStyleImage.h"

#include <cmath>

#include <QTextStream>
#include <QStringList>

#include <vtkObjectFactory.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkCallbackCommand.h>

#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkPointPicker.h>

#include <vtkPointData.h>
#include <vtkImageData.h>

#include <vtkPolyDataMapper.h>
#include <vtkDiskSource.h>
#include <vtkActor.h>
#include <vtkProperty.h>

#include <core/utility/vtkhelper.h>
#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
    , m_pointPicker(vtkSmartPointer<vtkPointPicker>::New())
    , m_highlightingActor(vtkSmartPointer<vtkActor>::New())
    , m_currentlyHighlighted(nullptr, -1)
    , m_mouseMoved(false)
{
    VTK_CREATE(vtkDiskSource, highlightingDisc);
    highlightingDisc->SetRadialResolution(128);
    highlightingDisc->SetCircumferentialResolution(128);
    highlightingDisc->SetInnerRadius(1);
    highlightingDisc->SetOuterRadius(2);

    VTK_CREATE(vtkPolyDataMapper, highlightingMapper);
    highlightingMapper->SetInputConnection(highlightingDisc->GetOutputPort());
    m_highlightingActor->SetMapper(highlightingMapper);

    m_highlightingActor->GetProperty()->SetColor(1, 0, 0);
    m_highlightingActor->PickableOff();
}

void InteractorStyleImage::OnMouseMove()
{
    Superclass::OnMouseMove();

    int* clickPos = GetInteractor()->GetEventPosition();
    m_pointPicker->Pick(clickPos[0], clickPos[1], 0, GetDefaultRenderer());

    sendPointInfo();

    m_mouseMoved = true;
}

void InteractorStyleImage::OnLeftButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    GrabFocus(EventCallbackCommand);

    StartPan();

    m_mouseMoved = false;
}

void InteractorStyleImage::OnLeftButtonUp()
{
    Superclass::OnLeftButtonUp();

    if (!m_mouseMoved)
        highlightPickedPoint();
    
    m_mouseMoved = false;
}

void InteractorStyleImage::OnMiddleButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyleImage::OnMiddleButtonUp()
{
    switch (State)
    {
    case VTKIS_DOLLY:
        EndDolly();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnMiddleButtonUp();
    }
}

void InteractorStyleImage::OnRightButtonDown()
{
    OnLeftButtonDown();
}

void InteractorStyleImage::OnRightButtonUp()
{
    OnLeftButtonUp();
}

void InteractorStyleImage::OnChar()
{
    // disable magic keys for now
}

void InteractorStyleImage::setRenderedData(const QList<RenderedData *> & renderedData)
{
    GetDefaultRenderer()->RemoveViewProp(m_highlightingActor);
    m_propToRenderedData.clear();

    for (RenderedData * r : renderedData)
    {
        vtkCollectionSimpleIterator it;
        r->viewProps()->InitTraversal(it);
        while (vtkProp * prop = r->viewProps()->GetNextProp(it))
        {
            if (prop->GetPickable())
                m_propToRenderedData.insert(prop, r);
        }
    }
}

void InteractorStyleImage::highlightPickedPoint()
{
    vtkIdType pointId = m_pointPicker->GetPointId();

    vtkProp * pickedProp = m_pointPicker->GetViewProp();
    if (!pickedProp)
    {
        highlightIndex(nullptr, -1);
        return;
    }

    RenderedData * renderedData = m_propToRenderedData.value(pickedProp);
    assert(renderedData);

    highlightIndex(renderedData->dataObject(), pointId);
    
    emit dataPicked(renderedData);

    emit indexPicked(renderedData->dataObject(), pointId);
}

void InteractorStyleImage::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    if (index == -1)
    {
        if (m_currentlyHighlighted.second < 0)
            return;

        GetDefaultRenderer()->RemoveViewProp(m_highlightingActor);
        GetDefaultRenderer()->GetRenderWindow()->Render();
        m_currentlyHighlighted = { nullptr, -1 };
        return;
    }

    assert(dataObject);

    if (m_currentlyHighlighted == QPair<DataObject *, vtkIdType>(dataObject, index))
        return;

    vtkImageData * image = vtkImageData::SafeDownCast(dataObject->dataSet());
    if (!image)
        return;

    double point[3];
    image->GetPoint(index, point);
    point[2] += 0.1;    // show in front of the image
    m_highlightingActor->SetPosition(point);

    GetDefaultRenderer()->AddViewProp(m_highlightingActor);
    GetDefaultRenderer()->GetRenderWindow()->Render();

    m_currentlyHighlighted = { dataObject, index };
}

void InteractorStyleImage::lookAtIndex(DataObject * /*polyData*/, vtkIdType /*index*/)
{
}

void InteractorStyleImage::sendPointInfo() const
{
    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(17);

    do
    {
        vtkAbstractMapper3D * mapper = m_pointPicker->GetMapper();
        if (!mapper)
            break;


        std::string inputname;

        vtkInformation * inputInfo = mapper->GetInformation();
        if (inputInfo->Has(DataObject::NameKey()))
            inputname = DataObject::NameKey()->Get(inputInfo);

        double * pos = m_pointPicker->GetPickPosition();

        vtkDataObject * dataObject = mapper->GetInputDataObject(0, 0);
        vtkImageData * image = vtkImageData::SafeDownCast(dataObject);

        if (!image)
            break;
        
        vtkIdType pointId = m_pointPicker->GetPointId();
        double value = image->GetPointData()->GetScalars()->GetTuple(pointId)[0];

        stream
            << "input file: " << QString::fromStdString(inputname) << endl
            << "row: " << pos[0] << endl
            << "column: " << pos[1] << endl
            << "value: " << value << endl;
    } while (false);

    QStringList info;
    QString line;
    while ((line = stream.readLine()) != QString())
        info.push_back(line);

    emit pointInfoSent(info);
}
