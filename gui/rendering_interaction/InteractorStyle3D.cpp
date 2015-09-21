#include "InteractorStyle3D.h"

#include <cassert>
#include <cmath>

#include <QTextStream>
#include <QStringList>
#include <QTime>
#include <QThread>
#include <QTimer>

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkMath.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolygon.h>
#include <vtkPolygon.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkVector.h>

#include <core/utility/vtkcamerahelper.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>

#include <gui/rendering_interaction/CameraDolly.h>
#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/Picker.h>


vtkStandardNewMacro(InteractorStyle3D);

InteractorStyle3D::InteractorStyle3D()
    : Superclass()
    , m_picker(std::make_unique<Picker>())
    , m_highlighter(std::make_unique<Highlighter>())
    , m_cameraDolly(std::make_unique<CameraDolly>())
    , m_mouseMoved(false)
{
}

InteractorStyle3D::~InteractorStyle3D() = default;

void InteractorStyle3D::OnMouseMove()
{
    Superclass::OnMouseMove();

    vtkVector2i clickPos;
    GetInteractor()->GetEventPosition(clickPos.GetData());
    FindPokedRenderer(clickPos[0], clickPos[1]);
    auto renderer = GetCurrentRenderer();
    assert(renderer);

    m_picker->pick(clickPos, *renderer);

    emit pointInfoSent(m_picker->pickedObjectInfo());

    m_mouseMoved = true;
}

void InteractorStyle3D::OnLeftButtonDown()
{
    Superclass::OnLeftButtonDown();

    m_mouseMoved = false;
}

void InteractorStyle3D::OnLeftButtonUp()
{
    Superclass::OnLeftButtonUp();

    if (!m_mouseMoved)
        highlightPickedIndex();

    m_mouseMoved = false;
}

void InteractorStyle3D::OnMiddleButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartDolly();
}

void InteractorStyle3D::OnMiddleButtonUp()
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

void InteractorStyle3D::OnRightButtonDown()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    if (!GetCurrentRenderer())
        return;

    StartPan();
}

void InteractorStyle3D::OnRightButtonUp()
{
    switch (State)
    {
    case VTKIS_PAN:
        EndPan();
        if (Interactor)
            ReleaseFocus();
        break;
    default:
        Superclass::OnRightButtonUp();
    }
}

void InteractorStyle3D::OnMouseWheelForward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(true);
}

void InteractorStyle3D::OnMouseWheelBackward()
{
    FindPokedRenderer(GetInteractor()->GetEventPosition()[0], GetInteractor()->GetEventPosition()[1]);

    MouseWheelDolly(false);
}

void InteractorStyle3D::OnChar()
{
    // disable most magic keys

    if (this->Interactor->GetKeyCode() == 'l')
        Superclass::OnChar();
}

void InteractorStyle3D::MouseWheelDolly(bool forward)
{
    if (!CurrentRenderer)
        return;

    GrabFocus(EventCallbackCommand);
    StartDolly();

    double factor = MotionFactor * 0.2 * MouseWheelMotionFactor;
    if (!forward)
        factor *= -1;
    factor = std::pow(1.1, factor);

    vtkCamera * camera = CurrentRenderer->GetActiveCamera();
    if (camera->GetParallelProjection())
    {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
    }
    else
    {
        camera->Dolly(factor);
        if (AutoAdjustCameraClippingRange)
        {
            CurrentRenderer->ResetCameraClippingRange();
        }
    }

    if (Interactor->GetLightFollowCamera())
    {
        CurrentRenderer->UpdateLightsGeometryToFollowCamera();
    }

    EndDolly();
    ReleaseFocus();

    Interactor->Render();
}

void InteractorStyle3D::highlightPickedIndex()
{
    // assume cells and points picked (in OnMouseMove)

    highlightIndex(m_picker->pickedDataObject(), m_picker->pickedIndex());

    if (auto vis = m_picker->pickedVisualizedData())
    {
        emit dataPicked(vis);
        emit indexPicked(&vis->dataObject(), m_picker->pickedIndex());
    }
}

DataObject * InteractorStyle3D::highlightedDataObject() const
{
    return m_highlighter->targetObject();
}

vtkIdType InteractorStyle3D::highlightedIndex() const
{
    return m_highlighter->lastTargetIndex();
}

void InteractorStyle3D::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    assert(index < 0 || dataObject);

    m_highlighter->setRenderer(GetCurrentRenderer());
    m_highlighter->setTarget(
        dataObject,
        index,
        m_picker->pickedIndexType());
}

void InteractorStyle3D::lookAtIndex(DataObject * dataObject, vtkIdType index)
{
    if (!dataObject)
        return;

    m_cameraDolly->setRenderer(GetCurrentRenderer());
    m_cameraDolly->moveTo(*dataObject, index, IndexType::cells);
}

void InteractorStyle3D::flashHightlightedCell(unsigned int milliseconds)
{
    m_highlighter->setFlashTimeMilliseconds(milliseconds);
    m_highlighter->flashTargets();
}
