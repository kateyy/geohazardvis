#pragma once

#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>

#include <QObject>

class vtkPointPicker;

class PickingInteractionStyle : public QObject, public vtkInteractorStyleTrackballCamera
{
    Q_OBJECT

public:
    explicit PickingInteractionStyle();

    static PickingInteractionStyle* New();
    vtkTypeMacro(PickingInteractionStyle, vtkInteractorStyleTrackballCamera);

    virtual void OnMouseMove() override;
    virtual void OnLeftButtonDown() override;

signals:
    void pointInfoSent(const QStringList &info) const;
    void pointClicked(int index) const;

protected:
    void pick();

    void sendPointInfo() const;

    vtkSmartPointer<vtkPointPicker> m_picker;
};
