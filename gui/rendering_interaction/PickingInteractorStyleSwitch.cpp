#include "PickingInteractorStyleSwitch.h"

#include <vtkObjectFactory.h>

#include <core/data_objects/DataObject.h>
#include <core/AbstractVisualizedData.h>


vtkStandardNewMacro(PickingInteractorStyleSwitch);


PickingInteractorStyleSwitch::PickingInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentPickingStyle(nullptr)
{
}

PickingInteractorStyleSwitch::~PickingInteractorStyleSwitch() = default;

DataObject * PickingInteractorStyleSwitch::highlightedDataObject() const
{
    if (!m_currentPickingStyle)
        return nullptr;
    
    return m_currentPickingStyle->highlightedDataObject();
}

vtkIdType PickingInteractorStyleSwitch::highlightedIndex() const
{
    if (!m_currentPickingStyle)
        return -1;

    return m_currentPickingStyle->highlightedIndex();
}

void PickingInteractorStyleSwitch::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    if (m_currentPickingStyle)
        m_currentPickingStyle->highlightIndex(dataObject, index);
}

void PickingInteractorStyleSwitch::lookAtIndex(DataObject * dataObject, vtkIdType index)
{
    if (m_currentPickingStyle)
        m_currentPickingStyle->lookAtIndex(dataObject, index);
}

void PickingInteractorStyleSwitch::styleAddedEvent(vtkInteractorStyle * style)
{
    IPickingInteractorStyle * pickingStyle = dynamic_cast<IPickingInteractorStyle*>(style);

    if (!m_currentPickingStyle)
        return;

    connect(pickingStyle, &IPickingInteractorStyle::pointInfoSent, this, &IPickingInteractorStyle::pointInfoSent);
    connect(pickingStyle, &IPickingInteractorStyle::dataPicked, this, &IPickingInteractorStyle::dataPicked);
    connect(pickingStyle, &IPickingInteractorStyle::indexPicked, this, &IPickingInteractorStyle::indexPicked);
}

void PickingInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentPickingStyle = dynamic_cast<IPickingInteractorStyle *>(currentStyle());
}
