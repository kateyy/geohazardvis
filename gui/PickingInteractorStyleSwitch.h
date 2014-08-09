#pragma once

#include "IPickingInteractorStyle.h"
#include "InteractorStyleSwitch.h"


class PickingInteractorStyleSwitch : public IPickingInteractorStyle, public InteractorStyleSwitch
{
    Q_OBJECT

public:
    static PickingInteractorStyleSwitch * New();
    vtkTypeMacro(PickingInteractorStyleSwitch, InteractorStyleSwitch);

    void setRenderedDataList(const QList<RenderedData *> * renderedData) override;

public slots:
    void highlightCell(vtkIdType cellId, DataObject * dataObject) override;
    void lookAtCell(vtkPolyData * polyData, vtkIdType cellId) override;
    
protected:
    PickingInteractorStyleSwitch();
    ~PickingInteractorStyleSwitch() override;

    void styleAdded(vtkInteractorStyle * style) override;

private:
    const QList<RenderedData *> * m_renderedData;
};
