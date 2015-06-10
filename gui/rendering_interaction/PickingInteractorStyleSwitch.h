#pragma once

#include <QList>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>
#include <gui/rendering_interaction/InteractorStyleSwitch.h>


class GUI_API PickingInteractorStyleSwitch : public IPickingInteractorStyle, public InteractorStyleSwitch
{
public:
    static PickingInteractorStyleSwitch * New();
    vtkTypeMacro(PickingInteractorStyleSwitch, InteractorStyleSwitch);

    void setRenderedData(const QList<RenderedData *> & renderedData) override;

    DataObject * highlightedDataObject() const override;
    vtkIdType highlightedIndex() const override;

    void highlightIndex(DataObject * dataObject, vtkIdType index) override;
    void lookAtIndex(DataObject * dataObject, vtkIdType index) override;
    
protected:
    PickingInteractorStyleSwitch();
    ~PickingInteractorStyleSwitch() override;

    void styleAddedEvent(vtkInteractorStyle * style) override;
    void currentStyleChangedEvent() override;

private:
    QList<RenderedData *> m_renderedData;
    IPickingInteractorStyle * m_currentPickingStyle;
};
