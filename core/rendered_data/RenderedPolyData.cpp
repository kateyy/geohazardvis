#include "RenderedPolyData.h"

#include <cassert>

#include <QDebug>

#include <vtkActor.h>
#include <vtkAlgorithmOutput.h>
#include <vtkCellCenters.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProp3DCollection.h>
#include <vtkProperty.h>
#include <vtkScalarsToColors.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/config.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/utility/DataExtent.h>

#if OPTION_ENABLE_TEXTURING
#include <vtkFieldData.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>
#include <core/TextureManager.h>
#include <core/filters/TransformTextureCoords.h>
#endif


using namespace reflectionzeug;


namespace
{
    enum class Interpolation
    {
        flat = VTK_FLAT, gouraud = VTK_GOURAUD, phong = VTK_PHONG
    };
    enum class SurfaceRepresentation
    {
        points = VTK_POINTS, wireframe = VTK_WIREFRAME, faces = VTK_SURFACE
    };
}

RenderedPolyData::RenderedPolyData(PolyDataObject & dataObject)
    : RenderedData3D(dataObject)
    , m_mapper{ vtkSmartPointer<vtkPolyDataMapper>::New() }
    , m_normals{ vtkSmartPointer<vtkPolyDataNormals>::New() }
    , m_transformedCellCenters{ vtkSmartPointer<vtkCellCenters>::New() }
{
    setupInformation(*m_mapper->GetInformation(), *this);

    // point normals required for correct lighting
    m_normals->ComputePointNormalsOn();
    m_normals->ComputeCellNormalsOff();
    m_mapper->SetInputConnection(m_normals->GetOutputPort());

    // don't break the lut configuration
    m_mapper->UseLookupTableScalarRangeOn();

    m_transformedCellCenters->SetInputConnection(transformedCoordinatesOutputPort());

    /** Changing vertex/index data may result in changed bounds */
    connect(this, &AbstractVisualizedData::geometryChanged, this, &RenderedPolyData::invalidateVisibleBounds);
}

RenderedPolyData::~RenderedPolyData() = default;

PolyDataObject & RenderedPolyData::polyDataObject()
{
    return static_cast<PolyDataObject &>(dataObject());
}

const PolyDataObject & RenderedPolyData::polyDataObject() const
{
    return static_cast<const PolyDataObject &>(dataObject());
}

vtkAlgorithmOutput * RenderedPolyData::transformedCellCenterOutputPort()
{
    return m_transformedCellCenters->GetOutputPort();
}

vtkDataSet * RenderedPolyData::transformedCellCenterDataSet()
{
    m_transformedCellCenters->Update();
    return m_transformedCellCenters->GetOutput();
}

std::unique_ptr<PropertyGroup> RenderedPolyData::createConfigGroup()
{
    auto renderSettings = RenderedData3D::createConfigGroup();

    renderSettings->addProperty<Color>("Color",
        [this] () {
        double * color = renderProperty()->GetColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this] (const Color & color) {
        renderProperty()->SetColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    });

    auto surfaceRepr = renderSettings->addProperty<SurfaceRepresentation>("SurfaceRepresentation",
        [this] () {
        return static_cast<SurfaceRepresentation>(renderProperty()->GetRepresentation());
    },
        [this] (SurfaceRepresentation rep) {
        renderProperty()->SetRepresentation(static_cast<int>(rep));
        emit geometryChanged();
    });
    surfaceRepr->setStrings({
        { SurfaceRepresentation::points, "Points" },
        { SurfaceRepresentation::wireframe, "Wireframe" },
        { SurfaceRepresentation::faces, "Faces" }
    });
    surfaceRepr->setOption("title", "Surface Representation");

    renderSettings->addProperty<bool>("edgesVisible",
        [this]() {
        return renderProperty()->GetEdgeVisibility() != 0;
    },
        [this](bool vis) {
        renderProperty()->SetEdgeVisibility(vis);
        emit geometryChanged();
    })
        ->setOption("title", "Edges");

    renderSettings->addProperty<unsigned>("edgeWidth",
        [this] () {
        return static_cast<unsigned>(renderProperty()->GetLineWidth());
    },
        [this] (unsigned width) {
        renderProperty()->SetLineWidth(static_cast<float>(width));
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Edge Width" },
            { "minimum", 1 },
            { "maximum", std::numeric_limits<unsigned>::max() },
            { "step", 1 },
            { "suffix", " pixel" }
    });

    renderSettings->addProperty<Color>("edgeColor",
        [this]() {
        double * color = renderProperty()->GetEdgeColor();
        return Color(static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255));
    },
        [this](const Color & color) {
        renderProperty()->SetEdgeColor(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0);
        emit geometryChanged();
    })
        ->setOption("title", "Edge Color");

    renderSettings->addProperty<unsigned>("pointSize",
        [this] () {
        return static_cast<unsigned>(renderProperty()->GetPointSize());
    },
        [this] (unsigned pointSize) {
        renderProperty()->SetPointSize(static_cast<float>(pointSize));
        emit geometryChanged();
    })
        ->setOptions({
            { "title", "Point Size" },
            { "minimum", 1 },
            { "maximum", 20 },
            { "step", 1 },
            { "suffix", " pixel" }
    });

    renderSettings->addProperty<bool>("lightingEnabled",
        std::bind(&vtkProperty::GetLighting, renderProperty()),
        [this](bool enabled) {
        renderProperty()->SetLighting(enabled);
        emit geometryChanged();
    })
        ->setOption("title", "Lighting");

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

    renderSettings->addProperty<double>("Transparency",
        [this]() {
        return (1.0 - renderProperty()->GetOpacity()) * 100;
    },
        [this](double transparency) {
        renderProperty()->SetOpacity(1.0 - transparency * 0.01);
        emit geometryChanged();
    })
        ->setOptions({
            { "minimum", 0 },
            { "maximum", 100 },
            { "step", 1 },
            { "suffix", " %" }
    });

    renderSettings->addProperty<bool>("BackfaceCulling",
        [this] () { return renderProperty()->GetBackfaceCulling() != 0; },
        [this] (bool backfaceCulling) {
        renderProperty()->SetBackfaceCulling(backfaceCulling);
        emit geometryChanged();
    })
        ->setOption("title", "Back-face culling");

#if OPTION_ENABLE_TEXTURING
    renderSettings->addProperty<FilePath>("OverlayTexture",
        [this] () { return texture().toStdString(); },
        [this] (const FilePath & filePath) {
        QString file = QString::fromStdString(filePath.toString());
        setTexture(file);
        emit geometryChanged();
    })
        ->setOption("title", "Overlay Texture");
#endif

    return renderSettings;
}

const QString & RenderedPolyData::texture() const
{
    return m_textureFileName;
}

#if OPTION_ENABLE_TEXTURING
void RenderedPolyData::setTexture(const QString & fileName)
{

    if (m_textureFileName == fileName)
    {
        return;
    }

    m_textureFileName = fileName;

    auto tex = TextureManager::fromFile(m_textureFileName);
    if (tex)
    {
        tex->RepeatOff();
    }

    mainActor()->SetTexture(tex);

}
#else
void RenderedPolyData::setTexture(const QString & /*fileName*/) {}
#endif

vtkSmartPointer<vtkProperty> RenderedPolyData::createDefaultRenderProperty() const
{
    auto prop = vtkSmartPointer<vtkProperty>::New();
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
    {
        return m_mainActor;
    }

    finalizePipeline();

    m_mainActor = vtkSmartPointer<vtkActor>::New();
    m_mainActor->SetMapper(m_mapper);
    m_mainActor->SetProperty(renderProperty());

    return m_mainActor;
}

void RenderedPolyData::scalarsForColorMappingChangedEvent()
{
    RenderedData3D::scalarsForColorMappingChangedEvent();

    // no mapping yet, so just render the data set
    if (!currentColorMappingData())
    {
        m_colorMappingOutput = processedOutputPort()->GetProducer();
        finalizePipeline();
        return;
    }

    currentColorMappingData()->configureMapper(*this, *m_mapper);

    vtkSmartPointer<vtkAlgorithm> filter;

    if (currentColorMappingData()->usesFilter())
    {
        filter = currentColorMappingData()->createFilter(*this);
        m_colorMappingOutput = filter;
    }
    else
    {
        m_colorMappingOutput = processedOutputPort()->GetProducer();
    }

    finalizePipeline();
}

void RenderedPolyData::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    m_mapper->SetLookupTable(currentColorMappingGradient());
}

void RenderedPolyData::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    mainActor()->SetVisibility(visible);
}

DataBounds RenderedPolyData::updateVisibleBounds()
{
    DataBounds bounds;
    mainActor()->GetBounds(bounds.data());
    return bounds;
}

void RenderedPolyData::finalizePipeline()
{
    // no color mapping set up yet
    if (!m_colorMappingOutput)
    {
        // disabled color mapping and texturing by default
        m_colorMappingOutput = processedOutputPort()->GetProducer();
        m_normals->SetInputConnection(m_colorMappingOutput->GetOutputPort());
        m_mapper->ScalarVisibilityOff();
    }

    vtkSmartPointer<vtkAlgorithm> upstreamAlgorithm = m_colorMappingOutput;

#if OPTION_ENABLE_TEXTURING
    auto textureCoords = vtkSmartPointer<vtkTextureMapToPlane>::New();
    textureCoords->SetInputConnection(upstreamAlgorithm);
    textureCoords->SetNormal(0, 0, 1);
    upstreamAlgorithm = textureCoords;

    auto demBoundsArray = vtkDoubleArray::FastDownCast(
        dataObject().dataSet()->GetFieldData()->GetAbstractArray("DEM_Bounds"));
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

        upstreamAlgorithm = transformTexCoords;
    }
#endif

    m_normals->SetInputConnection(upstreamAlgorithm->GetOutputPort());
}
