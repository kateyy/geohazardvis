#pragma once

#include <QObject>
#include <vtkSmartPointer.h>

class vtkPointPicker;
class vtkObject;
class vtkCommand;

class QStringList;

class PickingInfo : public QObject
{
    Q_OBJECT

public:
    void sendPointInfo(vtkSmartPointer<vtkPointPicker> picker, bool mouseClick) const;

signals:
    void infoSent(const QStringList &info) const;
    void selectionChanged(vtkObject* caller, unsigned long vtk_event, void* clientData, void* callData, vtkCommand* command) const;
};
