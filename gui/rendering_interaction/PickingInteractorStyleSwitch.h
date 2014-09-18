#pragma once

#include <QList>

#include "IPickingInteractorStyle.h"
#include "InteractorStyleSwitch.h"


class PickingInteractorStyleSwitch : public IPickingInteractorStyle, public InteractorStyleSwitch
{
    Q_OBJECT

public:
    static PickingInteractorStyleSwitch * New();
    vtkTypeMacro(PickingInteractorStyleSwitch, InteractorStyleSwitch);

    void setRenderedData(QList<RenderedData *> renderedData) override;

public slots:
    void highlightCell(DataObject * dataObject, vtkIdType cellId) override;
    void lookAtCell(DataObject * dataObject, vtkIdType cellId) override;
    
protected:
    PickingInteractorStyleSwitch();
    ~PickingInteractorStyleSwitch() override;

    void styleAddedEvent(vtkInteractorStyle * style) override;
    void currentStyleChangedEvent() override;

private:
    QList<RenderedData *> m_renderedData;
    IPickingInteractorStyle * m_currentPickingStyle;
};
