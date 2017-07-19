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

#include <core/rendered_data/RenderedData.h>


class vtkAlgorithm;
class vtkAssignAttribute;
class ImageMapToColors;
class vtkImageProperty;
class vtkImageSlice;
class vtkImageSliceMapper;

class ArrayChangeInformationFilter;
class DEMApplyShadingToColors;
class DEMImageNormals;
class DEMShadingFilter;
class ImageDataObject;


class CORE_API RenderedImageData : public RenderedData
{
public:
    explicit RenderedImageData(ImageDataObject & dataObject);
    ~RenderedImageData() override;

    ImageDataObject & imageDataObject();
    const ImageDataObject & imageDataObject() const;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    bool isInterpolationEnabled() const;
    void setInterpolationEnabled(bool enable);

    bool isShadingEnabled() const;
    void setEnableShading(bool enable);

    double ambient() const;
    void setAmbient(double ambient);

    double diffuse() const;
    void setDiffuse(double diffuse);


protected:
    vtkSmartPointer<vtkPropCollection> fetchViewProps() override;
    vtkImageSlice * slice();

    vtkImageProperty * property();

    void setupColorMapping(ColorMapping & colorMapping) override;
    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    DataBounds updateVisibleBounds() override;

private:
    void initializePipeline();
    void configureVisPipeline();

private:
    bool m_isShadingEnabled;

    vtkSmartPointer<ArrayChangeInformationFilter> m_copyScalarsFilter;
    vtkSmartPointer<ImageMapToColors> m_imageScalarsToColors;
    QMetaObject::Connection m_updateComponentConnection;

    vtkSmartPointer<vtkAlgorithm> m_colorMappingFilter;

    vtkSmartPointer<vtkAssignAttribute> m_assignElevationsForNormalComputation;
    vtkSmartPointer<DEMImageNormals> m_demNormals;
    vtkSmartPointer<DEMShadingFilter> m_demShading;
    vtkSmartPointer<vtkAssignAttribute> m_assignMappedColorsForShading;
    vtkSmartPointer<DEMApplyShadingToColors> m_applyDEMShading;

    vtkSmartPointer<vtkImageSliceMapper> m_mapper;
    vtkSmartPointer<vtkImageSlice> m_slice;
    vtkSmartPointer<vtkImageProperty> m_property;

private:
    Q_DISABLE_COPY(RenderedImageData)
};
