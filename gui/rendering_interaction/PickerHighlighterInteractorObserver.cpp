#include <gui/rendering_interaction/PickerHighlighterInteractorObserver.h>

#include <cassert>

#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVector.h>

#include <gui/rendering_interaction/Highlighter.h>
#include <gui/rendering_interaction/Picker.h>


vtkStandardNewMacro(PickerHighlighterInteractorObserver);


PickerHighlighterInteractorObserver::PickerHighlighterInteractorObserver()
    : vtkInteractorObserver()
    , m_callbackTag(0u)
    , m_mouseMoved(false)
    , m_picker(std::make_unique<Picker>())
    , m_highlighter(std::make_unique<Highlighter>())
{
    connect(m_highlighter.get(), &Highlighter::geometryChanged,
            this, &PickerHighlighterInteractorObserver::geometryChanged);
}

PickerHighlighterInteractorObserver::~PickerHighlighterInteractorObserver() = default;

void PickerHighlighterInteractorObserver::SetEnabled(int enabling)
{
    Superclass::SetEnabled(enabling);

    if (enabling != 0 && !this->Interactor)
    {
        vtkErrorMacro(<< "The interactor must be set prior to enabling/disabling the PickerHighlighterInteractorObserver");
        return;
    }

    this->Enabled = enabling;
}

void PickerHighlighterInteractorObserver::SetInteractor(vtkRenderWindowInteractor * interactor)
{
    if (this->Interactor == interactor)
    {
        return;
    }

    if (this->Interactor)
    {
        this->Interactor->RemoveObserver(this->m_callbackTag);
        m_highlighter->clear();
    }
    
    if (interactor)
    {
        this->m_callbackTag = interactor->AddObserver(vtkCommand::AnyEvent, this, &PickerHighlighterInteractorObserver::EventCallback);
    }

    Superclass::SetInteractor(interactor);
}

Picker & PickerHighlighterInteractorObserver::picker()
{
    return *m_picker;
}

const Picker & PickerHighlighterInteractorObserver::picker() const
{
    return *m_picker;
}

Highlighter & PickerHighlighterInteractorObserver::highlighter()
{
    return *m_highlighter;
}

const Highlighter & PickerHighlighterInteractorObserver::highlighter() const
{
    return *m_highlighter;
}

void PickerHighlighterInteractorObserver::EventCallback(vtkObject * /*subject*/, unsigned long eventId, void * /*userData*/)
{
    if (!this->Enabled)
        return;

    switch (eventId)
    {
    case vtkCommand::LeftButtonPressEvent:
        m_mouseMoved = false;
        break;
    case vtkCommand::MouseMoveEvent:
        m_mouseMoved = true;
        break;
    case vtkCommand::LeftButtonReleaseEvent:
    case vtkCommand::AnyEvent:

        if (!m_mouseMoved)
        {
            pickHighlight();
        }

        break;
    }
}

void PickerHighlighterInteractorObserver::pickHighlight()
{
    if (!this->Interactor)
        return;

    vtkVector2i clickPos;
    this->Interactor->GetEventPosition(clickPos.GetData());
    auto renderer = this->Interactor->FindPokedRenderer(clickPos[0], clickPos[1]);
    assert(renderer);

    if (!renderer)
        return;

    m_picker->pick(clickPos, *renderer);

    m_highlighter->setRenderer(renderer);
    m_highlighter->setTarget(
        m_picker->pickedVisualizedData(),
        0,
        m_picker->pickedIndex(),
        m_picker->pickedIndexType());

    emit dataPicked(m_picker->pickedVisualizedData(), m_picker->pickedIndex(), m_picker->pickedIndexType());
    emit pickedInfoChanged(m_picker->pickedObjectInfo());
}
