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
        m_imageSliceMappers[i] = vtkSmartPointer<vtkImageSliceMapper>::New();
        m_imageSliceMappers[i]->SetInputConnection(dataObject->processedOutputPort());
        m_imageSliceMappers[i]->SetOrientation(i);  // 0, 1, 2 maps to X, Y, Z

        int min = m_imageSliceMappers[i]->GetSliceNumberMinValue();
        int max = m_imageSliceMappers[i]->GetSliceNumberMaxValue();

        setSlicePosition(i, (max - min) / 2);

        m_imageSlicesScalars[i] = vtkSmartPointer<vtkImageSlice>::New();
        m_imageSlicesScalars[i]->SetMapper(m_imageSliceMappers[i]);

        VTK_CREATE(vtkImageProperty, property);
        property->UseLookupTableScalarRangeOn();
        m_imageSlicesScalars[i]->SetProperty(property);

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

    auto orop_sampleRate = configGroup->addProperty<std::array<int, 3>>("sampleRate",
        [this] (size_t i) { return m_extractVOI->GetSampleRate()[i]; },
        [this] (size_t i, int value) {
        int rates[3];
        m_extractVOI->GetSampleRate(rates);
        rates[i] = value;
        setSampleRate(rates[0], rates[1], rates[2]);
        emit geometryChanged();
    });
    orop_sampleRate->setOption("title", "sample rate");
    std::function<void(Property<int> &)> optionSetter = [] (Property<int> & prop){
        prop.setOption("minimum", 1);
    };
    orop_sampleRate->forEach(optionSetter);

    auto prop_slicesVisible = configGroup->addProperty<std::array<bool, 3>>("SlicesVisible",
        [this] (size_t i) { return m_slicesEnabled[i]; },
        [this] (size_t i, bool value) {
        m_slicesEnabled[i] = value;
        updateVisibilities();
    });
    prop_slicesVisible->setOption("title", "slices visible");
    auto slicePositions = configGroup->addGroup("SlicePosition");
    slicePositions->setOption("title", "slice positions");

    for (int i = 0; i < 3; ++i)
    {
        std::string axis = { char('X' + i) };

        prop_slicesVisible->asCollection()->at(i)->setOption("title", axis);

        vtkImageSliceMapper * mapper = m_imageSliceMappers[i];

        auto * slice_prop = slicePositions->addProperty<int>(axis + "slice",
            [mapper] () { return mapper->GetSliceNumber(); },
            [this, i] (int value) {
            setSlicePosition(i, value);
            emit geometryChanged();
        });
        slice_prop->setOption("title", axis);
        slice_prop->setOption("minimum", mapper->GetSliceNumberMinValue());
        slice_prop->setOption("maximum", mapper->GetSliceNumberMaxValue());
    }

    auto prop_interpolation = configGroup->addProperty<Interpolation>("Interpolation",
        [this] () {
        return static_cast<Interpolation>(m_imageSlicesScalars[0]->GetProperty()->GetInterpolationType());
    },
        [this] (Interpolation interpolation) {
        m_imageSlicesScalars[0]->GetProperty()->SetInterpolationType(static_cast<int>(interpolation));
        emit geometryChanged();
    });
    prop_interpolation->setStrings({
            { Interpolation::nearest, "nearest" },
            { Interpolation::linear, "linear" },
            { Interpolation::cubic, "cubic" }
    });

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

vtkSmartPointer<vtkProp3DCollection> RenderedVectorGrid3D::fetchViewProps3D()
{
    auto props = RenderedData3D::fetchViewProps3D();

    for (auto prop : m_imageSlicesScalars)
        props->AddItem(prop);

    return props;
}

void RenderedVectorGrid3D::scalarsForColorMappingChangedEvent()
{
    updateVisibilities();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    for (auto slice : m_imageSlicesScalars)
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
        m_imageSlicesScalars[i]->SetVisibility(isVisible() && showSliceI);
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

    m_imageSliceMappers[axis]->SetSliceNumber(slicePosition);
}
