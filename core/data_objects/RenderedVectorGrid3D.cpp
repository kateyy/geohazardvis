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
#include <vtkAssignAttribute.h>

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

    m_extractVOI->SetInputConnection(dataObject->processedOutputPort());

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


    VTK_CREATE(vtkAssignAttribute, assignVectorToScalars);
    assignVectorToScalars->SetInputConnection(dataObject->processedOutputPort());
    assignVectorToScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);

    for (int i = 0; i < 3; ++i)
    {
        m_extractSlices[i] = vtkSmartPointer<vtkExtractVOI>::New();
        m_extractSlices[i]->SetInputConnection(assignVectorToScalars->GetOutputPort());

        int voi[6];
        image->GetExtent(voi);
        voi[2 * i] = voi[2 * i + 1] = (voi[2 * i + 1] - voi[2 * i]) / 2;
        m_extractSlices[i]->SetVOI(voi);
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

    auto * zSlice = configGroup->addProperty<int>("zSlice",
        [this] () { return m_extractSlices[2]->GetVOI()[4]; },
        [this] (int value) {
        int voi[6];
        m_extractSlices[2]->GetVOI(voi);
        voi[4] = voi[5] = value;
        m_extractSlices[2]->SetVOI(voi);
        emit geometryChanged();
    });
    zSlice->setOption("minimum", 0);
    zSlice->setOption("maximum", static_cast<vtkImageData *>(dataObject()->processedDataSet())->GetExtent()[5]);

    return configGroup;
}

vtkProperty * RenderedVectorGrid3D::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    /*prop->SetColor(1, 0, 0);
    prop->SetInterpolationToFlat();*/
    prop->LightingOff();

    return prop;
}

vtkActor * RenderedVectorGrid3D::createActor()
{
    m_extractSlices[2]->Update();
    vtkImageData * slice = static_cast<vtkImageData *>(m_extractSlices[2]->GetOutput());

    VTK_CREATE(vtkLookupTable, lut);
    lut->SetTableRange(slice->GetScalarRange());
    lut->Build();

    VTK_CREATE(vtkTexture, texture);
    texture->SetLookupTable(lut);
    texture->SetInputConnection(m_extractSlices[2]->GetOutputPort());
    texture->MapColorScalarsThroughLookupTableOn();
    texture->InterpolateOn();

    const double * extent = slice->GetBounds();
    double xMin = extent[0], xMax = extent[1], yMin = extent[2], yMax = extent[3], zMin = extent[4], zMax = extent[5];

    VTK_CREATE(vtkPlaneSource, plane);
    plane->SetXResolution(slice->GetDimensions()[0]);
    plane->SetYResolution(slice->GetDimensions()[1]);
    plane->SetOrigin(xMin, yMin, zMin);
    // zSlice: xy-Plane
    plane->SetPoint1(xMax, yMin, zMin);
    plane->SetPoint2(xMin, yMax, zMin);

    VTK_CREATE(vtkAppendPolyData, append);
    append->AddInputConnection(m_glyph->GetOutputPort());
    append->AddInputConnection(plane->GetOutputPort());

    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(append->GetOutputPort());

    vtkActor * actor = vtkActor::New();
    actor->SetMapper(mapper);
    actor->SetTexture(texture);

    return actor;
}

void RenderedVectorGrid3D::setSampleRate(int x, int y, int z)
{
    m_extractVOI->SetSampleRate(x, y, z);

    m_extractVOI->Update();

    double cellSpacing = m_extractVOI->GetOutput()->GetSpacing()[0];
    m_glyph->SetScaleFactor(0.75 * m_extractVOI->GetOutput()->GetSpacing()[0]);
}
