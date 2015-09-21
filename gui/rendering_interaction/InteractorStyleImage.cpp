#include "InteractorStyleImage.h"

#include <cassert>
#include <cmath>

#include <vtkCallbackCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVector.h>

#include <core/AbstractVisualizedData.h>
#include <core/data_objects/DataObject.h>

#include <gui/rendering_interaction/CameraDolly.h>
#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/Picker.h>


vtkStandardNewMacro(InteractorStyleImage);

InteractorStyleImage::InteractorStyleImage()
    : Superclass()
    , m_picker(std::make_unique<Picker>())
    , m_highlighter(std::make_unique<Highlighter>())
    , m_camaraDolly(std::make_unique<CameraDolly>())
    , m_mouseMoved(false)
{
}

InteractorStyleImage::~InteractorStyleImage() = default;

void InteractorStyleImage::OnMouseMove()
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

void InteractorStyleImage::OnLeftButtonDown()
{
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
    int eventPos[2];
    GetInteractor()->GetEventPosition(eventPos);
    FindPokedRenderer(eventPos[0], eventPos[1]);

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

void InteractorStyleImage::highlightPickedPoint()
{
    highlightIndex(m_picker->pickedDataObject(), m_picker->pickedIndex());

    if (auto vis = m_picker->pickedVisualizedData())
    {
        emit dataPicked(vis);
        emit indexPicked(&vis->dataObject(), m_picker->pickedIndex());
    }
}

DataObject * InteractorStyleImage::highlightedDataObject() const
{
    return m_highlighter->targetObject();
}

vtkIdType InteractorStyleImage::highlightedIndex() const
{
    return m_highlighter->lastTargetIndex();
}

void InteractorStyleImage::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    assert(index < 0 || dataObject);

    m_highlighter->setRenderer(GetCurrentRenderer());
    m_highlighter->setTarget(
        dataObject,
        index,
        m_picker->pickedIndexType());
}

void InteractorStyleImage::lookAtIndex(DataObject * dataObject, vtkIdType index)
{
    if (!dataObject)
        return;

    m_camaraDolly->setRenderer(GetCurrentRenderer());
    m_camaraDolly->moveTo(*dataObject, index, IndexType::points);
}
