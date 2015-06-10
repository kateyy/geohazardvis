#include "PickingInteractorStyleSwitch.h"

#include <cassert>

#include <QStringList>

#include <vtkObjectFactory.h>

#include <core/data_objects/DataObject.h>
#include <core/rendered_data/RenderedData.h>


vtkStandardNewMacro(PickingInteractorStyleSwitch);


PickingInteractorStyleSwitch::PickingInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentPickingStyle(nullptr)
    , m_highlightedObject(nullptr)
    , m_highlightedIndex(-1)
{
}

PickingInteractorStyleSwitch::~PickingInteractorStyleSwitch() = default;

void PickingInteractorStyleSwitch::setRenderedData(const QList<RenderedData *> & renderedData)
{
    m_renderedData = renderedData;

    if (m_currentPickingStyle)
        m_currentPickingStyle->setRenderedData(m_renderedData);

    m_highlightedObject = nullptr;
    m_highlightedIndex = -1;
}

DataObject * PickingInteractorStyleSwitch::highlightedObject()
{
    return m_highlightedObject;
}

vtkIdType PickingInteractorStyleSwitch::highlightedIndex()
{
    return m_highlightedIndex;
}

void PickingInteractorStyleSwitch::highlightIndex(DataObject * dataObject, vtkIdType index)
{
    m_highlightedObject = dataObject;
    m_highlightedIndex = index;

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

    pickingStyle->setRenderedData(m_renderedData);

    connect(pickingStyle, &IPickingInteractorStyle::pointInfoSent, this, &IPickingInteractorStyle::pointInfoSent);
    connect(pickingStyle, &IPickingInteractorStyle::dataPicked, this, &IPickingInteractorStyle::dataPicked);
    connect(pickingStyle, &IPickingInteractorStyle::indexPicked, this, &IPickingInteractorStyle::indexPicked);
}

void PickingInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentPickingStyle = dynamic_cast<IPickingInteractorStyle *>(currentStyle());
}
