#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


template<typename T> class QList;
class vtkCamera;
class vtkObject;
class vtkRenderer;


class CORE_API vtkCameraSynchronization
{
public:
    vtkCameraSynchronization();
    ~vtkCameraSynchronization();

    void setEnabled(bool enabled);

    void add(vtkRenderer * renderer);
    void remove(vtkRenderer * renderer);
    void set(const QList<vtkRenderer *> & renderers);
    void clear();

private:
    void cameraChanged(vtkObject * source, unsigned long event, void * userData);

private:
    bool m_isEnabled;
    bool m_currentlySyncing;

    QMap<vtkSmartPointer<vtkCamera>, unsigned long> m_cameras;
    QMap<vtkCamera *, vtkRenderer *> m_renderers;
};
