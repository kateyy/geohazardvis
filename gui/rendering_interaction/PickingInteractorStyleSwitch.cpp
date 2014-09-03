#include "PickingInteractorStyleSwitch.h"

#include <cassert>

#include <QStringList>

#include <vtkObjectFactory.h>
#include <vtkActor.h>

#include <core/data_objects/DataObject.h>


vtkStandardNewMacro(PickingInteractorStyleSwitch);


PickingInteractorStyleSwitch::PickingInteractorStyleSwitch()
    : InteractorStyleSwitch()
{
}

PickingInteractorStyleSwitch::~PickingInteractorStyleSwitch() = default;

void PickingInteractorStyleSwitch::setRenderedDataList(const QList<RenderedData *> * renderedData)
{
    m_renderedData = renderedData;

    for (auto & it : namedStyles())
        dynamic_cast<IPickingInteractorStyle*>(it.second.Get())->setRenderedDataList(m_renderedData);
}

void PickingInteractorStyleSwitch::highlightCell(vtkIdType cellId, DataObject * dataObject)
{
    dynamic_cast<IPickingInteractorStyle*>(currentStyle())->highlightCell(cellId, dataObject);
}

void PickingInteractorStyleSwitch::lookAtCell(DataObject * dataObject, vtkIdType cellId)
{
    dynamic_cast<IPickingInteractorStyle*>(currentStyle())->lookAtCell(dataObject, cellId);
}

void PickingInteractorStyleSwitch::styleAdded(vtkInteractorStyle * style)
{
    IPickingInteractorStyle * pickingStyle = dynamic_cast<IPickingInteractorStyle*>(style);
    assert(pickingStyle);

    pickingStyle->setRenderedDataList(m_renderedData);

    connect(pickingStyle, &IPickingInteractorStyle::pointInfoSent, this, &IPickingInteractorStyle::pointInfoSent);
    connect(pickingStyle, &IPickingInteractorStyle::actorPicked, this, &IPickingInteractorStyle::actorPicked);
    connect(pickingStyle, &IPickingInteractorStyle::cellPicked, this, &IPickingInteractorStyle::cellPicked);
}
