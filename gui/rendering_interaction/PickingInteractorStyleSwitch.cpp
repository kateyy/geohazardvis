#include "PickingInteractorStyleSwitch.h"

#include <cassert>

#include <QStringList>

#include <vtkObjectFactory.h>

#include <core/data_objects/DataObject.h>
#include <core/data_objects/RenderedData.h>


vtkStandardNewMacro(PickingInteractorStyleSwitch);


PickingInteractorStyleSwitch::PickingInteractorStyleSwitch()
    : InteractorStyleSwitch()
    , m_currentPickingStyle(nullptr)
{
}

PickingInteractorStyleSwitch::~PickingInteractorStyleSwitch() = default;

void PickingInteractorStyleSwitch::setRenderedData(QList<RenderedData *> renderedData)
{
    m_renderedData = renderedData;

    if (m_currentPickingStyle)
        m_currentPickingStyle->setRenderedData(m_renderedData);
}

void PickingInteractorStyleSwitch::highlightCell(DataObject * dataObject, vtkIdType cellId)
{
    if (m_currentPickingStyle)
        m_currentPickingStyle->highlightCell(dataObject, cellId);
}

void PickingInteractorStyleSwitch::lookAtCell(DataObject * dataObject, vtkIdType cellId)
{
    if (m_currentPickingStyle)
        m_currentPickingStyle->lookAtCell(dataObject, cellId);
}

void PickingInteractorStyleSwitch::styleAddedEvent(vtkInteractorStyle * style)
{
    IPickingInteractorStyle * pickingStyle = dynamic_cast<IPickingInteractorStyle*>(style);

    if (!m_currentPickingStyle)
        return;

    pickingStyle->setRenderedData(m_renderedData);

    connect(pickingStyle, &IPickingInteractorStyle::pointInfoSent, this, &IPickingInteractorStyle::pointInfoSent);
    connect(pickingStyle, &IPickingInteractorStyle::dataPicked, this, &IPickingInteractorStyle::dataPicked);
    connect(pickingStyle, &IPickingInteractorStyle::cellPicked, this, &IPickingInteractorStyle::cellPicked);
}

void PickingInteractorStyleSwitch::currentStyleChangedEvent()
{
    m_currentPickingStyle = dynamic_cast<IPickingInteractorStyle *>(currentStyle());
}
