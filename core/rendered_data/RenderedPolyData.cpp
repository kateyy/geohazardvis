#include "RenderedPolyData.h"

#include <cassert>

#include <QImage>

#include <vtkInformation.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkInformationStringKey.h>

#include <vtkAlgorithmOutput.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkScalarsToColors.h>

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkProp3DCollection.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageData.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/TextureManager.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/filters/TransformTextureCoords.h>
#include <core/color_mapping/ColorMappingData.h>


using namespace reflectionzeug;


namespace
{
    enum class Interpolation
    {
        flat = VTK_FLAT, gouraud = VTK_GOURAUD, phong = VTK_PHONG
    };
    enum class Representation
    {
        points = VTK_POINTS, wireframe = VTK_WIREFRAME, surface = VTK_SURFACE
    };
}

RenderedPolyData::RenderedPolyData(PolyDataObject & dataObject)
    : RenderedData3D(dataObject)
    , m_mapper(vtkSmartPointer<vtkPolyDataMapper>::New())
    , m_normals(vtkSmartPointer<vtkPolyDataNormals>::New())
{
    assert(vtkPolyData::SafeDownCast(dataObject.dataSet()));

    auto & mapperInfo = *m_mapper->GetInformation();
    mapperInfo.Set(DataObject::NameKey(), dataObject.name().toUtf8().data());
    DataObject::storePointer(mapperInfo, &dataObject);
    AbstractVisualizedData::storePointer(mapperInfo, this);

    m_normals->ComputePointNormalsOn();
    m_normals->ComputeCellNormalsOff();

    // disabled color mapping by default
    m_colorMappingOutput = dataObject.processedOutputPort();
    m_normals->SetInputConnection(m_colorMappingOutput);

    m_mapper->ScalarVisibilityOff();
    m_mapper->SetInputConnection(m_normals->GetOutputPort());

    // don't break the lut configuration
    m_mapper->UseLookupTableScalarRangeOn();
}

PolyDataObject & RenderedPolyData::polyDataObject()
{
    return static_cast<PolyDataObject &>(dataObject());
}

const PolyDataObject & RenderedPolyData::polyDataObject() const
{
    return static_cast<const PolyDataObject &>(dataObject());
}

reflectionzeug::PropertyGroup * RenderedPolyData::createConfigGroup()
{
    PropertyGroup * renderSettings = new PropertyGroup();

    renderSettings->addProperty<Color>("color",
        [this] () {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

    renderSettings->addProperty<FilePath>("DemTexture",
        [this] () { return texture().toStdString(); },
        [this] (const FilePath & filePath) {
        QString file = QString::fromStdString(filePath.toString());
        setTexture(file);
        emit geometryChanged();
    })->setOption("title", "DEM overlay texture");


    auto * edgesVisible = renderSettings->addProperty<bool>("edgesVisible",
        [this]() {
        return (renderProperty()->GetEdgeVisibility() == 0) ? false : true;
    },
        [this](bool vis) {
        renderProperty()->SetEdgeVisibility(vis);
        emit geometryChanged();
    });
    edgesVisible->setOption("title", "Edges");

    auto * lineWidth = renderSettings->addProperty<unsigned>("lineWidth",
        [this] () {
        return static_cast<unsigned>(renderProperty()->GetLineWidth());
    },
        [this] (unsigned width) {
        renderProperty()->SetLineWidth(static_cast<float>(width));
        emit geometryChanged();
    });
    lineWidth->setOption("title", "Edge Width");
    lineWidth->setOption("minimum", 1);
    lineWidth->setOption("maximum", std::numeric_limits<unsigned>::max());
    lineWidth->setOption("step", 1);
    lineWidth->setOption("suffix", " pixel");

    auto * edgeColor = renderSettings->addProperty<Color>("edgeColor",
        [this]() {
        double * color = renderProperty()->GetEdgeColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetEdgeColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });
    edgeColor->setOption("title", "Edge Color");


    auto * representation = renderSettings->addProperty<Representation>("Representation",
        [this]() {
        return static_cast<Representation>(renderProperty()->GetRepresentation());
    },
        [this](const Representation & rep) {
        renderProperty()->SetRepresentation(static_cast<int>(rep));
        emit geometryChanged();
    });
    representation->setStrings({
            { Representation::points, "points" },
            { Representation::wireframe, "wireframe" },
            { Representation::surface, "surface" }
    });

    auto * lightingEnabled = renderSettings->addProperty<bool>("lightingEnabled",
        std::bind(&vtkProperty::GetLighting, renderProperty()),
        [this](bool enabled) {
        renderProperty()->SetLighting(enabled);
        emit geometryChanged();
    });
    lightingEnabled->setOption("title", "Lighting");

    auto * interpolation = renderSettings->addProperty<Interpolation>("ShadingModel",
        [this]() {
        return static_cast<Interpolation>(renderProperty()->GetInterpolation());
    },
        [this](const Interpolation & i) {
        renderProperty()->SetInterpolation(static_cast<int>(i));
        emit geometryChanged();
    });
    interpolation->setOption("title", "Shading Model");
    interpolation->setStrings({
            { Interpolation::flat, "flat" },
            //{ Interpolation::gouraud, "gouraud" },
            { Interpolation::phong, "phong" }
    });

    auto transparency = renderSettings->addProperty<double>("Transparency",
        [this]() {
        return (1.0 - renderProperty()->GetOpacity()) * 100;
    },
        [this](double transparency) {
        renderProperty()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    });
    transparency->setOption("minimum", 0);
    transparency->setOption("maximum", 100);
    transparency->setOption("step", 1);
    transparency->setOption("suffix", " %");

    auto backfaceCulling = renderSettings->addProperty<bool>("BackfaceCulling",
        [this] () { return renderProperty()->GetBackfaceCulling() != 0; },
        [this] (bool backfaceCulling) {
        renderProperty()->SetBackfaceCulling(backfaceCulling);
        emit geometryChanged();
    });
    backfaceCulling->setOption("title", "Back-face culling");

    auto pointSize = renderSettings->addProperty<unsigned>("pointSize",
        [this]() {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this](unsigned pointSize) {
        renderProperty()->SetPointSize(static_cast<float>(pointSize));
        emit geometryChanged();
    });
    pointSize->setOption("title", "Point Size");
    pointSize->setOption("minimum", 1);
    pointSize->setOption("maximum", 20);
    pointSize->setOption("step", 1);
    pointSize->setOption("suffix", " pixel");

    return renderSettings;
}

const QString & RenderedPolyData::texture() const
{
    return m_textureFileName;
}

void RenderedPolyData::setTexture(const QString & fileName)
{
    if (m_textureFileName == fileName)
        return;

    m_textureFileName = fileName;

    auto tex = TextureManager::fromFile(m_textureFileName);
    if (tex)
    {
        tex->RepeatOff();
    }

    mainActor()->SetTexture(tex);
}

vtkProperty * RenderedPolyData::createDefaultRenderProperty() const
{
    vtkProperty * prop = vtkProperty::New();
    prop->SetColor(0, 0.6, 0);
    prop->SetOpacity(1.0);
    prop->SetInterpolationToFlat();
    prop->SetEdgeVisibility(true);
    prop->SetEdgeColor(0.1, 0.1, 0.1);
    prop->SetLineWidth(1.2f);
    prop->SetBackfaceCulling(false);
    prop->SetLighting(false);

    return prop;
}

vtkSmartPointer<vtkProp3DCollection> RenderedPolyData::fetchViewProps3D()
{
    auto actors = RenderedData3D::fetchViewProps3D();
    actors->AddItem(mainActor());

    return actors;
}

vtkActor * RenderedPolyData::mainActor()
{
    if (m_mainActor)
        return m_mainActor;

    m_mainActor = vtkSmartPointer<vtkActor>::New();
    m_mainActor->SetMapper(m_mapper);
    m_mainActor->SetProperty(renderProperty());

    return m_mainActor;
}

void RenderedPolyData::scalarsForColorMappingChangedEvent()
{
    RenderedData3D::scalarsForColorMappingChangedEvent();

    // no mapping yet, so just render the data set
    if (!m_colorMappingData)
    {
        m_colorMappingOutput = dataObject().processedOutputPort();
        finalizePipeline();
        return;
    }

    m_colorMappingData->configureMapper(this, m_mapper);

    vtkSmartPointer<vtkAlgorithm> filter;

    if (m_colorMappingData->usesFilter())
    {
        filter = m_colorMappingData->createFilter(this);
        m_colorMappingOutput = filter->GetOutputPort();
    }
    else
        m_colorMappingOutput = dataObject().processedOutputPort();

    finalizePipeline();
}

void RenderedPolyData::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    m_mapper->SetLookupTable(m_gradient);
}

void RenderedPolyData::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    mainActor()->SetVisibility(visible);
}

void RenderedPolyData::finalizePipeline()
{
    auto textureCoords = vtkSmartPointer<vtkTextureMapToPlane>::New();
    textureCoords->SetInputConnection(m_colorMappingOutput);
    textureCoords->SetNormal(0, 0, 1);

    vtkDoubleArray * demBoundsArray = vtkDoubleArray::SafeDownCast(
        dataObject().dataSet()->GetFieldData()->GetArray("DEM_Bounds"));
    if (demBoundsArray)
    {
        assert(demBoundsArray->GetNumberOfComponents() * demBoundsArray->GetNumberOfTuples() == 6);
        double * demBounds =  demBoundsArray->GetPointer(0);
        double demSize[2] = { demBounds[1] - demBounds[0], demBounds[3] - demBounds[2] };
        double demCenter[2] = {
            0.5 * (demBounds[0] + demBounds[1]),
            0.5 * (demBounds[2] + demBounds[3]) };

        double thisBounds[6];
        dataObject().bounds(thisBounds);
        double thisSize[2] = { thisBounds[1] - thisBounds[0], thisBounds[3] - thisBounds[2] };
        double thisCenter[2] = {
            0.5 * (thisBounds[0] + thisBounds[1]),
            0.5 * (thisBounds[2] + thisBounds[3]) };

        auto transformTexCoords = vtkSmartPointer<TransformTextureCoords>::New();
        transformTexCoords->SetInputConnection(textureCoords->GetOutputPort());
        transformTexCoords->SetScale(   // scale tex coords from surface size to DEM texture size
            thisSize[0] / demSize[0],
            thisSize[1] / demSize[1],
            1);
        transformTexCoords->SetPosition(    // in DEM texture space: translate to actual dem position
            (thisCenter[0] - demCenter[0]) / demSize[0],
            (thisCenter[1] - demCenter[1]) / demSize[1],
            0);

        m_normals->SetInputConnection(transformTexCoords->GetOutputPort());
    }
    else
    {
        m_normals->SetInputConnection(textureCoords->GetOutputPort());
    }
}

vtkAlgorithmOutput * RenderedPolyData::colorMappingOutput()
{
    return m_colorMappingOutput;
}

vtkPolyDataMapper * RenderedPolyData::mapper()
{
    return m_mapper;
}
