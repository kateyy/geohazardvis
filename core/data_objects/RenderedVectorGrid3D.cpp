#include "RenderedVectorGrid3D.h"

#include <algorithm>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkLookupTable.h>

#include <vtkLineSource.h>
#include <vtkPlaneSource.h>
#include <vtkAppendPolyData.h>

#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkDataSetAttributes.h>

#include <vtkExtractVOI.h>
#include <vtkGlyph3D.h>

#include <vtkPolyDataMapper.h>

#include <vtkProperty.h>
#include <vtkActor.h>


#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


using namespace reflectionzeug;


namespace
{

const vtkIdType DefaultMaxNumberOfPoints = 1000;

vtkSmartPointer<vtkAlgorithm> createArrow()
{
    VTK_CREATE(vtkLineSource, shaft);
    shaft->SetPoint1(0.f, 0.f, 0.f);
    shaft->SetPoint2(1.f, 0.f, 0.f);

    VTK_CREATE(vtkLineSource, cone1);
    cone1->SetPoint1(1.00f, 0.0f, 0.f);
    cone1->SetPoint2(0.65f, 0.1f, 0.f);

    VTK_CREATE(vtkLineSource, cone2);
    cone2->SetPoint1(1.00f,  0.0f, 0.f);
    cone2->SetPoint2(0.65f, -0.1f, 0.f);

    VTK_CREATE(vtkAppendPolyData, arrow);
    arrow->AddInputConnection(shaft->GetOutputPort());
    arrow->AddInputConnection(cone1->GetOutputPort());
    arrow->AddInputConnection(cone2->GetOutputPort());

    return arrow;
}

}

RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData(dataObject)
    , m_glyph(vtkSmartPointer<vtkGlyph3D>::New())
    , m_extractVOI(vtkSmartPointer<vtkExtractVOI>::New())
{
    assert(vtkImageData::SafeDownCast(dataObject->processedDataSet()));

    vtkImageData * image = static_cast<vtkImageData *>(dataObject->processedDataSet());

    m_extractVOI->SetInputData(dataObject->dataSet());

    vtkSmartPointer<vtkAlgorithm> arrow = createArrow();
    
    m_glyph->SetInputConnection(m_extractVOI->GetOutputPort());
    m_glyph->SetSourceConnection(arrow->GetOutputPort());
    m_glyph->ScalingOn();
    m_glyph->SetScaleModeToDataScalingOff();
    m_glyph->SetVectorModeToUseVector();


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

    auto * renderSettings = configGroup->addGroup("renderSettings");
    renderSettings->setOption("title", "rendering");

    auto * color = renderSettings->addProperty<Color>("lineColor",
        [this]() {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    color->setOption("title", "line color");

    auto pointSize = renderSettings->addProperty<unsigned>("pointSize",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this](unsigned pointSize) {
        renderProperty()->SetPointSize(pointSize);
        emit geometryChanged();
    });
    pointSize->setOption("title", "point size");
    pointSize->setOption("minimum", 1);
    pointSize->setOption("maximum", 100);
    pointSize->setOption("step", 1);

    auto lineWidth = renderSettings->addProperty<unsigned>("lineWidth",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetLineWidth());
    },
        [this](unsigned lineWidth) {
        renderProperty()->SetLineWidth(lineWidth);
        emit geometryChanged();
    });
    lineWidth->setOption("title", "line width");
    lineWidth->setOption("minimum", 1);
    lineWidth->setOption("maximum", 100);
    lineWidth->setOption("step", 1);
    
    auto arrowLength = renderSettings->addProperty<float>("arrowLength",
        [this]() { return static_cast<float>(m_glyph->GetScaleFactor());  },
        [this](float value) {
        m_glyph->SetScaleFactor(value);
        emit geometryChanged();
    });
    arrowLength->setOption("title", "arrow length");
    arrowLength->setOption("step", 0.02f);

    for (int i = 0; i < 3; ++i)
    {
        std::string axis = { char('x' + i) };

        auto * slice_prop = configGroup->addProperty<int>(axis + "Slice",
            [this, i] () { return m_extractSlices[i]->GetVOI()[2 * i]; },
            [this, i] (int value) {
            setSlicePosition(i, value);
            emit geometryChanged();
        });
        slice_prop->setOption("minimum", 0);
        slice_prop->setOption("maximum", static_cast<vtkImageData *>(dataObject()->processedDataSet())->GetExtent()[2 * i + 1]);
    }

    auto * sampleRate = configGroup->addProperty<std::array<int, 3>>("sampleRate",
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

    return configGroup;
}

vtkProperty * RenderedVectorGrid3D::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(1, 0, 0);
    prop->SetInterpolationToFlat();
    prop->LightingOff();

    return prop;
}

vtkActor * RenderedVectorGrid3D::createActor()
{
    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(m_glyph->GetOutputPort());

    vtkActor * actor = vtkActor::New();
    actor->SetMapper(mapper);

    return actor;
}

QList<vtkActor *> RenderedVectorGrid3D::fetchAttributeActors()
{
    QList<vtkActor *> actors;
    for (auto actor : m_sliceActors)
        actors << actor;

    return actors;
}

void RenderedVectorGrid3D::gradientForColorMappingChangedEvent()
{
    for (auto sliceActor : m_sliceActors)
    {
        vtkTexture * texture = sliceActor->GetTexture();
        assert(texture);
        texture->SetLookupTable(m_lut);
    }
}

void RenderedVectorGrid3D::setSampleRate(int x, int y, int z)
{
    m_extractVOI->SetSampleRate(x, y, z);

    m_extractVOI->Update();

    double cellSpacing = m_extractVOI->GetOutput()->GetSpacing()[0];
    m_glyph->SetScaleFactor(0.75 * m_extractVOI->GetOutput()->GetSpacing()[0]);
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
