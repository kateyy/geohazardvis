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

#include <array>

#include <core/rendered_data/RenderedData3D.h>
#include <core/utility/DataExtent.h>


class vtkAlgorithm;
class vtkAlgorithmOutput;
class vtkExtractVOI;
class vtkImageData;
class vtkLookupTable;
class vtkRenderWindowInteractor;

class ImagePlaneWidget;
class VectorGrid3DDataObject;


class CORE_API RenderedVectorGrid3D : public RenderedData3D
{
    Q_OBJECT

public:
    explicit RenderedVectorGrid3D(VectorGrid3DDataObject & dataObject);
    ~RenderedVectorGrid3D() override;

    /** the interactor needs to be set in order to use the image plane widgets */
    void setRenderWindowInteractor(vtkRenderWindowInteractor * interactor);

    VectorGrid3DDataObject & vectorGrid3DDataObject();
    const VectorGrid3DDataObject & vectorGrid3DDataObject() const;

    std::unique_ptr<reflectionzeug::PropertyGroup> createConfigGroup() override;

    void setSampleRate(int x, int y, int z);
    void sampleRate(int sampleRate[3]);
    vtkImageData * resampledDataSet();
    vtkAlgorithmOutput * resampledOuputPort();

    int slicePosition(int axis);
    void setSlicePosition(int axis, int slicePosition);

    unsigned int numberOfOutputPorts() const override;

signals:
    void sampleRateChanged(int x, int y, int z);

protected:
    vtkAlgorithmOutput * processedOutputPortInternal(unsigned int port) override;

    vtkSmartPointer<vtkProp3DCollection> fetchViewProps3D() override;

    void scalarsForColorMappingChangedEvent() override;
    void colorMappingGradientChangedEvent() override;
    void visibilityChangedEvent(bool visible) override;

    DataBounds updateVisibleBounds() override;

    void updatePlaneLUT();

private:
    void initializePipeline();
    void updateVisibilities();

private:
    bool m_isInitialized;

    // for vector mapping
    vtkSmartPointer<vtkExtractVOI> m_extractVOI;

    // for color mapping / LIC2D planes

    std::array<vtkSmartPointer<ImagePlaneWidget>, 3> m_planeWidgets;
    std::array<vtkSmartPointer<vtkAlgorithm>, 3> m_colorMappingInputs;
    vtkSmartPointer<vtkLookupTable> m_blackWhiteLUT;

    std::array<bool, 3> m_slicesEnabled;
    std::array<int, 3> m_storedSliceIndexes;

    ImageExtent m_dataExtent;

private:
    Q_DISABLE_COPY(RenderedVectorGrid3D)
};
