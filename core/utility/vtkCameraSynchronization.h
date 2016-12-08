#pragma once

#include <map>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <core/core_api.h>


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
    void set(const std::vector<vtkRenderer *> & renderers);
    void clear();

private:
    void cameraChanged(vtkObject * source, unsigned long event, void * userData);

private:
    bool m_isEnabled;
    bool m_currentlySyncing;

    std::map<vtkSmartPointer<vtkCamera>, unsigned long> m_cameras;
    std::map<vtkCamera *, vtkWeakPointer<vtkRenderer>> m_renderers;

private:
    vtkCameraSynchronization(const vtkCameraSynchronization &) = delete;
    void operator=(const vtkCameraSynchronization &) = delete;
};
