#pragma once

#include <memory>

#include <QMap>

#include <vtkInteractorStyleTerrain.h>
#include <vtkSmartPointer.h>

#include <gui/rendering_interaction/IPickingInteractorStyle.h>


class QTime;
class QTimer;
class vtkPointPicker;
class vtkCellPicker;
class vtkPolyData;
class vtkProp;

class Highlighter;


class GUI_API InteractorStyle3D : public IPickingInteractorStyle, public vtkInteractorStyleTerrain
{
public:
    static InteractorStyle3D * New();
    vtkTypeMacro(InteractorStyle3D, vtkInteractorStyleTerrain);

    void setRenderedData(const QList<RenderedData *> & renderedData) override;

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;

    void OnChar() override;

    DataObject * highlightedDataObject() const override;
    vtkIdType highlightedIndex() const override;

    void highlightIndex(DataObject * dataObject, vtkIdType index) override;
    void lookAtIndex(DataObject * polyData, vtkIdType index) override;
    void flashHightlightedCell(unsigned int milliseconds = 2000u);

protected:
    explicit InteractorStyle3D();
    ~InteractorStyle3D();

    void MouseWheelDolly(bool forward);

    void highlightPickedIndex();
    void sendPointInfo() const;

protected:
    QMap<vtkProp *, RenderedData *> m_actorToRenderedData;

    vtkSmartPointer<vtkPointPicker> m_pointPicker;
    vtkSmartPointer<vtkCellPicker> m_cellPicker;

    std::unique_ptr<Highlighter> m_highlighter;

    bool m_mouseMoved;
};
