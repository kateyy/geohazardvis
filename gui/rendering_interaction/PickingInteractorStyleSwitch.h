#pragma once

#include <QList>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>
#include <gui/rendering_interaction/InteractorStyleSwitch.h>


class GUI_API PickingInteractorStyleSwitch : public IPickingInteractorStyle, public InteractorStyleSwitch
{
    Q_OBJECT

public:
    static PickingInteractorStyleSwitch * New();
    vtkTypeMacro(PickingInteractorStyleSwitch, InteractorStyleSwitch);

    void setRenderedData(QList<RenderedData *> renderedData) override;

    DataObject * highlightedObject();
    vtkIdType highlightedCell();

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

    DataObject * m_highlightedObject;
    vtkIdType m_highlightedCell;
};
