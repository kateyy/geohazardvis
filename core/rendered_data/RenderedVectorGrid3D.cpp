#include "RenderedVectorGrid3D.h"

#include <algorithm>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <vtkExtractVOI.h>

#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageProperty.h>

#include <vtkProp3DCollection.h>
#include <vtkScalarsToColors.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/scalar_mapping/ScalarsForColorMapping.h>


using namespace reflectionzeug;


namespace
{

const vtkIdType DefaultMaxNumberOfPoints = 1000;

enum Interpolation
{
    nearest = VTK_NEAREST_INTERPOLATION,
    linear = VTK_LINEAR_INTERPOLATION,
    cubic = VTK_CUBIC_INTERPOLATION
};

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

    for (int i = 0; i < 3; ++i)
    {

        m_sliceMappers[i] = vtkSmartPointer<vtkImageSliceMapper>::New();
        m_sliceMappers[i]->SetInputConnection(dataObject->processedOutputPort());
        m_sliceMappers[i]->SetOrientation(i);  // 0, 1, 2 maps to X, Y, Z
        
        int min = m_sliceMappers[i]->GetSliceNumberMinValue();
        int max = m_sliceMappers[i]->GetSliceNumberMaxValue();

        setSlicePosition(i, (max - min) / 2);

        m_slices[i] = vtkSmartPointer<vtkImageSlice>::New();
        m_slices[i]->SetMapper(m_sliceMappers[i]);

        VTK_CREATE(vtkImageProperty, property);
        property->UseLookupTableScalarRangeOn();
        property->SetDiffuse(0.8);
        property->SetAmbient(0.5);
        m_slices[i]->SetProperty(property);

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
    PropertyGroup * renderSettings = new PropertyGroup();

    auto group_glyphs = renderSettings->addGroup("Glyphs");
    {
        auto prop_sampleRate = group_glyphs->addProperty<std::array<int, 3>>("sampleRate",
            [this] (size_t i) { return m_extractVOI->GetSampleRate()[i]; },
            [this] (size_t i, int value) {
            int rates[3];
            m_extractVOI->GetSampleRate(rates);
            rates[i] = value;
            setSampleRate(rates[0], rates[1], rates[2]);
            emit geometryChanged();
        });
        prop_sampleRate->setOption("title", "Sample Rate");
        prop_sampleRate->forEach(std::function<void(Property<int> &)>( [](Property<int> & prop) {
            prop.setOption("minimum", 1);
        }));
    }

    auto group_scalarSlices = renderSettings->addGroup("Slices");
    {
        auto prop_visibilities = group_scalarSlices->addProperty<std::array<bool, 3>>("Visible",
            [this] (size_t i) { return m_slicesEnabled[i]; },
            [this] (size_t i, bool value) {
            m_slicesEnabled[i] = value;
            updateVisibilities();
        });

        auto prop_positions = group_scalarSlices->addGroup("Positions");

        for (int i = 0; i < 3; ++i)
        {
            std::string axis = { char('X' + i) };

            prop_visibilities->asCollection()->at(i)->setOption("title", axis);

            vtkImageSliceMapper * mapper = m_sliceMappers[i];

            auto * slice_prop = prop_positions->addProperty<int>(axis + "slice",
                [mapper] () { return mapper->GetSliceNumber(); },
                [this, i] (int value) {
                setSlicePosition(i, value);
                emit geometryChanged();
            });
            slice_prop->setOption("title", axis);
            slice_prop->setOption("minimum", mapper->GetSliceNumberMinValue());
            slice_prop->setOption("maximum", mapper->GetSliceNumberMaxValue());
        }

        vtkImageProperty * property = m_slices[0]->GetProperty();

        auto prop_transparency = group_scalarSlices->addProperty<double>("Transparency",
            [property ]() {
            return (1.0 - property->GetOpacity()) * 100;
        },
            [this] (double transparency) {
            for (auto slice : m_slices)
                slice->GetProperty()->SetOpacity(1.0 - transparency * 0.01);
            emit geometryChanged();
        });
        prop_transparency->setOption("minimum", 0);
        prop_transparency->setOption("maximum", 100);
        prop_transparency->setOption("step", 1);
        prop_transparency->setOption("suffix", " %");

        auto prop_diffLighting = group_scalarSlices->addProperty<double>("DiffuseLighting",
            [property] () { return property->GetDiffuse(); },
            [this] (double diff) {
            for (auto slice : m_slices)
                slice->GetProperty()->SetDiffuse(diff);
            emit geometryChanged();
        });
        prop_diffLighting->setOption("title", "Diffuse Lighting");
        prop_diffLighting->setOption("minimum", 0);
        prop_diffLighting->setOption("maximum", 1);
        prop_diffLighting->setOption("step", 0.05);

        auto prop_ambientLighting = group_scalarSlices->addProperty<double>("AmbientLighting",
            [property]() { return property->GetAmbient(); },
            [this] (double ambient) {
            for (auto slice : m_slices)
                slice->GetProperty()->SetAmbient(ambient);
            emit geometryChanged();
        });
        prop_ambientLighting->setOption("title", "Ambient Lighting");
        prop_ambientLighting->setOption("minimum", 0);
        prop_ambientLighting->setOption("maximum", 1);
        prop_ambientLighting->setOption("step", 0.05);

        auto prop_interpolation = group_scalarSlices->addProperty<Interpolation>("Interpolation",
            [property] () {
            return static_cast<Interpolation>(property->GetInterpolationType());
        },
            [this] (Interpolation interpolation) {
            for (auto slice : m_slices)
                slice->GetProperty()->SetInterpolationType(static_cast<int>(interpolation));
            emit geometryChanged();
        });
        prop_interpolation->setStrings({
                { Interpolation::nearest, "nearest" },
                { Interpolation::linear, "linear" },
                { Interpolation::cubic, "cubic" }
        });
    }

    return renderSettings;
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

vtkSmartPointer<vtkProp3DCollection> RenderedVectorGrid3D::fetchViewProps3D()
{
    auto props = RenderedData3D::fetchViewProps3D();

    for (auto prop : m_slices)
        props->AddItem(prop);

    return props;
}

void RenderedVectorGrid3D::scalarsForColorMappingChangedEvent()
{
    updateVisibilities();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    for (auto slice : m_slices)
    {
        auto property = slice->GetProperty();
        assert(property);
        property->SetLookupTable(m_gradient);
    }
}

void RenderedVectorGrid3D::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    updateVisibilities();
}

void RenderedVectorGrid3D::updateVisibilities()
{
    bool showSliceScalars = m_scalars && m_scalars->scalarsName() == QString::fromLatin1(dataObject()->dataSet()->GetPointData()->GetVectors()->GetName());

    for (int i = 0; i < 3; ++i)
    {
        bool showSliceI = showSliceScalars && m_slicesEnabled[i];
        m_slices[i]->SetVisibility(isVisible() && showSliceI);
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

    m_sliceMappers[axis]->SetSliceNumber(slicePosition);
}
