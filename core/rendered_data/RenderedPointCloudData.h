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

#include <QString>

#include <core/rendered_data/RenderedData3D.h>


class vtkActor;
class vtkAlgorithm;
class vtkPolyData;
class vtkPolyDataMapper;

class PointCloudDataObject;


class CORE_API RenderedPointCloudData : public RenderedData3D
{
public:
    explicit RenderedPointCloudData(PointCloudDataObject & dataObject);
    ~RenderedPointCloudData() override;

    PointCloudDataObject & pointCloudDataObject();
    const PointCloudDataObject & pointCloudDataObject() const;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    vtkActor * mainActor();

    vtkSmartPointer<vtkProperty> createDefaultRenderProperty() const override;

protected:
    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    void setupColorMapping(ColorMapping & colorMapping) override;
    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    DataBounds updateVisibleBounds() override;

    /** Integrate last pipeline steps. By default, this connects the colorMappingOutput
        with the mapper. */
    virtual void finalizePipeline();

private:
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_mainActor;

    vtkSmartPointer<vtkAlgorithm> m_colorMappingOutput;

private:
    Q_DISABLE_COPY(RenderedPointCloudData)
};
