#include "RenderedVectorGrid3D.h"

#include <algorithm>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkPlaneSource.h>
#include <vtkOutlineFilter.h>

#include <vtkImageData.h>
#include <vtkPointData.h>

#include <vtkExtractVOI.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkScalarsToColors.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/color_mapping/ColorMappingData.h>


using namespace reflectionzeug;


namespace
{

const vtkIdType DefaultMaxNumberOfPoints = 1000;

}

RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData3D(dataObject)
    , m_extractVOI(vtkSmartPointer<vtkExtractVOI>::New())
    , m_slicesEnabled()
{
    assert(vtkImageData::SafeDownCast(dataObject->processedDataSet()));

    vtkImageData * image = static_cast<vtkImageData *>(dataObject->processedDataSet());

    m_extractVOI->SetInputData(dataObject->dataSet());

    // initialize sample rate to prevent crashing/blocking rendering
    vtkIdType numPoints = image->GetNumberOfPoints();
    int sampleRate = std::max(1, (int)std::floor(std::cbrt(float(numPoints) / DefaultMaxNumberOfPoints)));
    setSampleRate(sampleRate, sampleRate, sampleRate);


    int extent[6];
    image->GetExtent(extent);

    for (int i = 0; i < 3; ++i)
    {
        m_extractSlices[i] = vtkSmartPointer<vtkExtractVOI>::New();
        m_extractSlices[i]->SetInputConnection(dataObject->processedOutputPort());

        VTK_CREATE(vtkTexture, texture);
        texture->SetInputConnection(m_extractSlices[i]->GetOutputPort());
        texture->MapColorScalarsThroughLookupTableOn();
        texture->InterpolateOn();

        m_slicePlanes[i] = vtkSmartPointer<vtkPlaneSource>::New();
        setSlicePosition(i, (extent[2 * i + 1] - extent[2 * i]) / 2);

        VTK_CREATE(vtkPolyDataMapper, mapper);
        mapper->SetInputConnection(m_slicePlanes[i]->GetOutputPort());

        m_sliceActors[i] = vtkSmartPointer<vtkActor>::New();
        m_sliceActors[i]->SetMapper(mapper);
        m_sliceActors[i]->SetTexture(texture);

        VTK_CREATE(vtkProperty, property);
        property->LightingOff();
        m_sliceActors[i]->SetProperty(property);


        VTK_CREATE(vtkOutlineFilter, sliceOutline);
        sliceOutline->SetInputConnection(m_slicePlanes[i]->GetOutputPort());

        VTK_CREATE(vtkPolyDataMapper, outlineMapper);
        outlineMapper->SetInputConnection(sliceOutline->GetOutputPort());
        outlineMapper->ScalarVisibilityOff();

        m_sliceOutlineActors[i] = vtkSmartPointer<vtkActor>::New();
        m_sliceOutlineActors[i]->SetMapper(outlineMapper);
        VTK_CREATE(vtkProperty, outlineProperty);
        m_sliceOutlineActors[i]->SetProperty(outlineProperty);
        outlineProperty->SetColor(0, 0, 0);

        m_slicesEnabled[i] = true;
    }
}

RenderedVectorGrid3D::~RenderedVectorGrid3D() = default;

VectorGrid3DDataObject * RenderedVectorGrid3D::vectorGrid3DDataObject()
{
    return static_cast<VectorGrid3DDataObject *>(dataObject());
}

const VectorGrid3DDataObject * RenderedVectorGrid3D::vectorGrid3DDataObject() const
{
    return static_cast<const VectorGrid3DDataObject *>(dataObject());
}

PropertyGroup * RenderedVectorGrid3D::createConfigGroup()
{
    PropertyGroup * configGroup = new PropertyGroup();

    auto sampleRate = configGroup->addProperty<std::array<int, 3>>("sampleRate",
        [this] (size_t i) { return m_extractVOI->GetSampleRate()[i]; },
        [this] (size_t i, int value) {
        int rates[3];
        m_extractVOI->GetSampleRate(rates);
        rates[i] = value;
        setSampleRate(rates[0], rates[1], rates[2]);
        emit geometryChanged();
    });
    sampleRate->setOption("title", "sample rate");
    std::function<void(Property<int> &)> optionSetter = [] (Property<int> & prop){
        prop.setOption("minimum", 1);
    };
    sampleRate->forEach(optionSetter);

    auto slicesVisible = configGroup->addProperty<std::array<bool, 3>>("SlicesVisible",
        [this] (size_t i) { return m_slicesEnabled[i]; },
        [this] (size_t i, bool value) {
        m_slicesEnabled[i] = value;
        updateVisibilities();
    });
    slicesVisible->setOption("title", "slices visible");
    auto slicePositions = configGroup->addGroup("SlicePosition");
    slicePositions->setOption("title", "slice positions");
    for (int i = 0; i < 3; ++i)
    {
        std::string axis = { char('X' + i) };

        slicesVisible->asCollection()->at(i)->setOption("title", axis);

        auto * slice_prop = slicePositions->addProperty<int>(axis + "slice",
            [this, i] () { return m_extractSlices[i]->GetVOI()[2 * i]; },
            [this, i] (int value) {
            setSlicePosition(i, value);
            emit geometryChanged();
        });
        slice_prop->setOption("title", axis);
        slice_prop->setOption("minimum", 0);
        slice_prop->setOption("maximum", static_cast<vtkImageData *>(dataObject()->processedDataSet())->GetExtent()[2 * i + 1]);
    }

    auto * lightingEnabled = configGroup->addProperty<bool>("lightingEnabled",
        [this] () {
        return m_sliceActors.front()->GetProperty()->GetLighting();
    },
        [this] (bool enabled) {
        for (auto & actor : m_sliceActors)
            actor->GetProperty()->SetLighting(enabled);
        emit geometryChanged();
    });
    lightingEnabled->setOption("title", "lighting enabled");

    return configGroup;
}

vtkImageData * RenderedVectorGrid3D::resampledDataSet()
{
    m_extractVOI->Update();
    return m_extractVOI->GetOutput();
}

vtkAlgorithmOutput * RenderedVectorGrid3D::resampledOuputPort()
{
    return m_extractVOI->GetOutputPort();
}

vtkSmartPointer<vtkActorCollection> RenderedVectorGrid3D::fetchActors()
{
    vtkSmartPointer<vtkActorCollection> actors = RenderedData3D::fetchActors();

    for (auto actor : m_sliceActors)
        actors->AddItem(actor);
    for (auto actor : m_sliceOutlineActors)
        actors->AddItem(actor);

    return actors;
}

void RenderedVectorGrid3D::scalarsForColorMappingChangedEvent()
{
    RenderedData3D::scalarsForColorMappingChangedEvent();

    updateVisibilities();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    for (auto sliceActor : m_sliceActors)
    {
        vtkTexture * texture = sliceActor->GetTexture();
        assert(texture);
        texture->SetLookupTable(m_gradient);
    }
}

void RenderedVectorGrid3D::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    updateVisibilities();
}

void RenderedVectorGrid3D::updateVisibilities()
{
    bool showSliceScalars = m_scalars && m_scalars->scalarsName() == QString::fromUtf8(dataObject()->dataSet()->GetPointData()->GetVectors()->GetName());

    for (int i = 0; i < 3; ++i)
    {
        bool showSliceI = showSliceScalars && m_slicesEnabled[i];
        m_sliceActors[i]->SetVisibility(isVisible() && showSliceI);
        m_sliceOutlineActors[i]->SetVisibility(isVisible() && !showSliceI);
    }

    emit geometryChanged();
}

void RenderedVectorGrid3D::setSampleRate(int x, int y, int z)
{
    m_extractVOI->SetSampleRate(x, y, z);
}

void RenderedVectorGrid3D::sampleRate(int sampleRate[3])
{
    m_extractVOI->GetSampleRate(sampleRate);
}

void RenderedVectorGrid3D::setSlicePosition(int axis, int slicePosition)
{
    assert(0 < axis || axis < 3);

    vtkExtractVOI * extractSlice = m_extractSlices[axis];

    int voi[6];
    static_cast<vtkImageData *>(dataObject()->processedDataSet())->GetExtent(voi);
    voi[2 * axis] = voi[2 * axis + 1] = slicePosition;
    extractSlice->SetVOI(voi);

    extractSlice->Update();
    vtkImageData * slice = extractSlice->GetOutput();
    double bounds[6];
    slice->GetBounds(bounds);
    double xMin = bounds[0], xMax = bounds[1], yMin = bounds[2], yMax = bounds[3], zMin = bounds[4], zMax = bounds[5];

    vtkPlaneSource * plane = m_slicePlanes[axis];
    plane->SetOrigin(xMin, yMin, zMin);

    switch (axis)
    {
    case 0: // x
        plane->SetXResolution(slice->GetDimensions()[1]);
        plane->SetYResolution(slice->GetDimensions()[2]);
        plane->SetPoint1(xMin, yMax, zMin);
        plane->SetPoint2(xMin, yMin, zMax);
        break;
    case 1: // y
        plane->SetXResolution(slice->GetDimensions()[0]);
        plane->SetYResolution(slice->GetDimensions()[2]);
        plane->SetPoint1(xMax, yMin, zMin);
        plane->SetPoint2(xMin, yMin, zMax);
        break;
    case 2: // z
        plane->SetXResolution(slice->GetDimensions()[0]);
        plane->SetYResolution(slice->GetDimensions()[1]);
        plane->SetPoint1(xMax, yMin, zMin);
        plane->SetPoint2(xMin, yMax, zMin);
        break;
    }
}
