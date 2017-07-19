/*
 * GeohazardVis
 * Copyright (C) 2017 Karsten Tausche <geodev@posteo.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
