#include "RenderedVectorGrid3D.h"

#include <algorithm>
#include <limits>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkProp3DCollection.h>
#include <vtkLookupTable.h>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <vtkAssignAttribute.h>
#include <vtkArrayCalculator.h>
#include <vtkExtractVOI.h>
#include <vtkCellPicker.h>
#include <vtkImageMapToColors.h>
#include <vtkImageOrthoPlanes.h>
#include <vtkImagePlaneWidget.h>
#include <vtkImageReslice.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLExtensionManager.h>
#include <vtkProperty.h>
#include <vtkTexture.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/filters/ArrayRenameFilter.h>
#include <core/filters/NoiseImageSource.h>
#include <core/filters/vtkImageDataLIC2D.h>


using namespace reflectionzeug;


namespace
{

const vtkIdType DefaultMaxNumberOfPoints = 1000;

enum ResliceInterpolation
{
    nearest = VTK_NEAREST_RESLICE,
    linear = VTK_LINEAR_RESLICE,
    cubic = VTK_CUBIC_RESLICE
};
enum NoiseSourceType
{
    cpuUniform = NoiseImageSource::CpuUniformDistribution,
    gpuPerlin = NoiseImageSource::GpuPerlinNoise
};

const std::string s_resliceOutputArray = "ImageScalars";
const std::string s_lic2DWithMangnitudes = "LIC2DWithMagnitudes";

}

RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject * dataObject)
    : RenderedData3D(dataObject)
    , m_isInitialized(false)
    , m_extractVOI(vtkSmartPointer<vtkExtractVOI>::New())
    , m_slicesEnabled()
{
    assert(vtkImageData::SafeDownCast(dataObject->processedDataSet()));

    vtkImageData * image = static_cast<vtkImageData *>(dataObject->processedDataSet());

    int extent[6];
    image->GetExtent(extent);

    m_extractVOI->SetInputData(dataObject->dataSet());

    // decrease sample rate to prevent crashing/blocking rendering
    vtkIdType numPoints = image->GetNumberOfPoints();
    int sampleRate = std::max(1, (int)std::floor(std::cbrt(float(numPoints) / DefaultMaxNumberOfPoints)));
    setSampleRate(sampleRate, sampleRate, sampleRate);

    std::string vectorsName;
    for (int vectorArray = 0; vectorArray < dataObject->dataSet()->GetPointData()->GetNumberOfArrays(); ++vectorArray)
        if (dataObject->dataSet()->GetPointData()->GetArray(vectorArray)->GetNumberOfComponents() == 3)
        {
            vectorsName = dataObject->dataSet()->GetPointData()->GetArray(vectorArray)->GetName();
            break;
        }

    std::string vectorsScaledName{ vectorsName + "_scaled" };

    m_glContext = vtkSmartPointer<vtkRenderWindow>::New();
    vtkOpenGLRenderWindow * openGLContext = vtkOpenGLRenderWindow::SafeDownCast(m_glContext);
    assert(openGLContext);
    openGLContext->GetExtensionManager()->IgnoreDriverBugsOn(); // required for Intel HD
    openGLContext->OffScreenRenderingOn();


    m_noiseImage = vtkSmartPointer<NoiseImageSource>::New();
    m_noiseImage->SetExtent(0, 1270, 0, 1270, 0, 0);
    m_noiseImage->SetValueRange(0, 1);
    m_noiseImage->SetNumberOfComponents(2);

    m_orthoPlanes = vtkSmartPointer<vtkImageOrthoPlanes>::New();

    m_texturePlaneProperty = vtkSmartPointer<vtkProperty>::New();
    m_texturePlaneProperty->LightingOn();
    m_texturePlaneProperty->SetInterpolationToFlat(); // interpolation is done by reslice

    VTK_CREATE(vtkCellPicker, planePicker); // shared picker for the plane widgets



    VTK_CREATE(vtkAssignAttribute, assignVectors);
    assignVectors->SetInputData(dataObject->dataSet());
    assignVectors->Assign(vectorsName.c_str(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
    assignVectors->Update();

    vtkDataSet * dataSetWithVectors = vtkDataSet::SafeDownCast(assignVectors->GetOutput());


    for (int i = 0; i < 3; ++i)
    {
        /** Reslice widgets  */

        m_planeWidgets[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        // TODO VTK Bug? A pipeline connection from vtkAssignAttribute does not work for some reason
        //m_planeWidgets[i]->SetInputConnection(dataObject->processedOutputPort());
        m_planeWidgets[i]->SetInputData(dataSetWithVectors);
        m_planeWidgets[i]->UserControlledLookupTableOn();
        m_planeWidgets[i]->RestrictPlaneToVolumeOn();

        // this is required to fix picking with multiple planes in a view
        m_planeWidgets[i]->SetPicker(planePicker);
        // this is recommended for rendering with other transparent objects
        m_planeWidgets[i]->GetColorMap()->SetOutputFormatToRGBA();
        m_planeWidgets[i]->GetColorMap()->PassAlphaToOutputOn();

        m_planeWidgets[i]->SetLeftButtonAction(vtkImagePlaneWidget::VTK_SLICE_MOTION_ACTION);
        m_planeWidgets[i]->SetRightButtonAction(vtkImagePlaneWidget::VTK_CURSOR_ACTION);
        m_planeWidgets[i]->SetTexturePlaneProperty(m_texturePlaneProperty);

        m_orthoPlanes->SetPlane(i, m_planeWidgets[i]);


        /** Line Integral Convolution 2D */

        VTK_CREATE(vtkAssignAttribute, assignVectors);
        assignVectors->SetInputConnection(m_planeWidgets[i]->GetReslice()->GetOutputPort());
        assignVectors->Assign(s_resliceOutputArray.c_str(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

        m_lic2DVectorScale[i] = vtkSmartPointer<vtkArrayCalculator>::New();
        m_lic2DVectorScale[i]->SetInputConnection(assignVectors->GetOutputPort());
        m_lic2DVectorScale[i]->AddVectorArrayName(s_resliceOutputArray.c_str());
        m_lic2DVectorScale[i]->SetResultArrayName(vectorsScaledName.c_str());

        VTK_CREATE(vtkAssignAttribute, assignScaledVectors);
        assignScaledVectors->SetInputConnection(m_lic2DVectorScale[i]->GetOutputPort());
        assignScaledVectors->Assign(vectorsScaledName.c_str(),
            vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

        m_lic2D[i] = vtkSmartPointer<vtkImageDataLIC2D>::New();
        m_lic2D[i]->SetInputConnection(0, assignScaledVectors->GetOutputPort());
        m_lic2D[i]->SetInputConnection(1, m_noiseImage->GetOutputPort());
        m_lic2D[i]->SetSteps(50);
        m_lic2D[i]->GlobalWarningDisplayOff();
        m_lic2D[i]->SetContext(m_glContext);

        int min = extent[2 * i];
        int max = extent[2 * i + 1];
        setSlicePosition(i, (max - min) / 2);

        m_slicesEnabled[i] = true;
    }

    double scalarRange[2] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest() };
    for (int i = 0; i < 3; ++i)
    {
        double r[2];
        image->GetPointData()->GetScalars()->GetRange(r, i);
        scalarRange[0] = std::min(scalarRange[0], r[0]);
        scalarRange[1] = std::max(scalarRange[1], r[1]);
    }
    setLic2DVectorScaleFactor(1.0 / std::max(std::abs(scalarRange[0]), std::abs(scalarRange[1])));

    setColorMode(ColorMode::UserDefined);

    m_isInitialized = true;
}

RenderedVectorGrid3D::~RenderedVectorGrid3D()
{
    for (auto & lic : m_lic2D)
        lic->SetContext(nullptr);
}

void RenderedVectorGrid3D::setRenderWindowInteractor(vtkRenderWindowInteractor * interactor)
{
    for (vtkImagePlaneWidget * widget : m_planeWidgets)
        widget->SetInteractor(interactor);

    updateVisibilities();
}

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

        int extent[6];
        static_cast<vtkImageData *>(dataObject()->dataSet())->GetExtent(extent);

        auto prop_positions = group_scalarSlices->addGroup("Positions");

        for (int i = 0; i < 3; ++i)
        {
            std::string axis = { char('X' + i) };

            prop_visibilities->asCollection()->at(i)->setOption("title", axis);

            auto * slice_prop = prop_positions->addProperty<int>(axis + "slice",
                [this, i] () { return slicePosition(i); },
                [this, i] (int value) {
                setSlicePosition(i, value);
                emit geometryChanged();
            });
            slice_prop->setOption("title", axis);
            slice_prop->setOption("minimum", extent[2 * i]);
            slice_prop->setOption("maximum", extent[2 * i + 1]);
        }

        vtkProperty * property = m_texturePlaneProperty;

        std::string depthWarning{ "Using transparency and lighting at the same time probably leads to rendering errors." };

        auto prop_transparency = group_scalarSlices->addProperty<double>("Transparency",
            [property]() {
            return (1.0 - property->GetOpacity()) * 100;
        },
            [this, property] (double transparency) {
            property->SetOpacity(1.0 - transparency * 0.01);
            emit geometryChanged();
        });
        prop_transparency->setOption("minimum", 0);
        prop_transparency->setOption("maximum", 100);
        prop_transparency->setOption("step", 1);
        prop_transparency->setOption("suffix", " %");
        prop_transparency->setOption("tooltip", depthWarning);

        auto prop_lighting = group_scalarSlices->addProperty<bool>("Lighting",
            [property]() { return property->GetLighting(); },
            [this, property](bool lighting) {
            property->SetLighting(lighting);
            emit geometryChanged();
        });
        prop_lighting->setOption("tooltip", depthWarning);

        auto prop_diffLighting = group_scalarSlices->addProperty<double>("DiffuseLighting",
            [property] () { return property->GetDiffuse(); },
            [this, property] (double diff) {
            property->SetDiffuse(diff);
            emit geometryChanged();
        });
        prop_diffLighting->setOption("title", "Diffuse Lighting");
        prop_diffLighting->setOption("minimum", 0);
        prop_diffLighting->setOption("maximum", 1);
        prop_diffLighting->setOption("step", 0.05);

        auto prop_ambientLighting = group_scalarSlices->addProperty<double>("AmbientLighting",
            [property]() { return property->GetAmbient(); },
            [this, property] (double ambient) {
            property->SetAmbient(ambient);
            emit geometryChanged();
        });
        prop_ambientLighting->setOption("title", "Ambient Lighting");
        prop_ambientLighting->setOption("minimum", 0);
        prop_ambientLighting->setOption("maximum", 1);
        prop_ambientLighting->setOption("step", 0.05);

        auto prop_interpolation = group_scalarSlices->addProperty<ResliceInterpolation>("Interpolation",
            [this] () {
            return static_cast<ResliceInterpolation>(m_planeWidgets[0]->GetResliceInterpolate());
        },
            [this] (ResliceInterpolation interpolation) {
            for (auto plane : m_planeWidgets)
                plane->SetResliceInterpolate(static_cast<int>(interpolation));
            emit geometryChanged();
        });
        prop_interpolation->setStrings({
                { ResliceInterpolation::nearest, "nearest" },
                { ResliceInterpolation::linear, "linear" },
                { ResliceInterpolation::cubic, "cubic" }
        });
    }

    auto group_LIC2D = renderSettings->addGroup("LIC2D");
    group_LIC2D->setOption("title", "Line Integral Convolution 2D");
    {

        auto prop_noiseSource = group_LIC2D->addProperty<NoiseSourceType>("NoiseSource",
            [this] () { return static_cast<NoiseSourceType>(m_noiseImage->GetNoiseSource()); },
            [this] (NoiseSourceType type) {
            m_noiseImage->SetNoiseSource(static_cast<int>(type));
            emit geometryChanged();
        });
        prop_noiseSource->setOption("title", "Noise Source");
        prop_noiseSource->setStrings({
            { NoiseSourceType::cpuUniform, "CPU (uniform distribution)" },
            { NoiseSourceType::gpuPerlin, "GPU (Perlin Noise" }
        });

        auto prop_steps = group_LIC2D->addProperty<int>("Step",
            [this] () { return m_lic2D[0]->GetSteps(); },
            [this] (int s) {
            for (int i = 0; i < 3; ++i)
            {
                m_lic2D[i]->SetSteps(s);
                forceLICUpdate(i);
            }
            emit geometryChanged();
        });
        prop_steps->setOption("minimum", 1);

        auto prop_stepSize = group_LIC2D->addProperty<double>("StepSize",
            [this] () { return m_lic2D[0]->GetStepSize(); },
            [this] (double size) {
            for (int i = 0; i < 3; ++i)
            {
                m_lic2D[i]->SetStepSize(size);
                forceLICUpdate(i);
            }
            emit geometryChanged();
        });
        prop_stepSize->setOption("title", "Step Size");
        prop_stepSize->setOption("minimum", 0.001);
        prop_stepSize->setOption("precision", 3);
        prop_stepSize->setOption("step", 0.1);


        // bug somewhere in VTK?
        //auto prop_magnification = group_LIC2D->addProperty<int>("Magnification",
        //    [this] () { return m_lic2D[0]->GetMagnification(); },
        //    [this] (int mag) {
        //    for (int i = 0; i < 3; ++i)
        //    {
        //        m_lic2D[i]->SetMagnification(mag);
        //        forceLICUpdate(i);
        //    }
        //    emit geometryChanged();
        //});
        //prop_magnification->setOption("minimum", 1);

        auto prop_vectorScale = group_LIC2D->addProperty<float>("vectorScaleFactor",
            [this] () { return m_lic2DVectorScaleFactor; },
            [this] (float f) {
            setLic2DVectorScaleFactor(f);
            emit geometryChanged();
        });
        prop_vectorScale->setOption("title", "Vector Scale Factor");
        prop_vectorScale->setOption("minimum", 0);

        auto prop_noiseSize = group_LIC2D->addProperty<int>("NoiseSize",
            [this] () { return m_noiseImage->GetExtent()[1]; },
            [this] (int size) {
            m_noiseImage->SetExtent(0, size, 0, size, 0, 0);
            for (int i = 0; i < 3; ++i)
                forceLICUpdate(i);
            emit geometryChanged();
        });
        prop_noiseSize->setOption("minimum", 1);
        prop_noiseSize->setOption("title", "Noise Image Size");
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

    return props;
}

void RenderedVectorGrid3D::scalarsForColorMappingChangedEvent()
{
    RenderedData3D::scalarsForColorMappingChangedEvent();

    ColorMode newMode = ColorMode::UserDefined;

    if (m_scalars && m_scalars->name() == "LIC 2D")
    {
        newMode = ColorMode::LIC;
    }
    else if (m_scalars && m_scalars->usesFilter())
    {
        vtkSmartPointer<vtkAlgorithm> filter = vtkSmartPointer<vtkAlgorithm>::Take(m_scalars->createFilter(this));
        filter->Update();
        vtkDataSet * dataSet = vtkDataSet::SafeDownCast(filter->GetOutputDataObject(0));
        assert(dataSet);

        if (dataSet->GetPointData()->GetScalars())
        {
            for (auto & plane : m_planeWidgets)
                plane->SetInputData(dataSet);
            
            newMode = ColorMode::ScalarMapping;
        }
    }

    if (newMode != ColorMode::ScalarMapping)
        for (auto & plane : m_planeWidgets)
            plane->SetInputData(dataObject()->processedDataSet());


    resetSlicePositions();

    setColorMode(newMode);

    updateVisibilities();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    vtkSmartPointer<vtkLookupTable> lut = vtkLookupTable::SafeDownCast(m_gradient);
    assert(lut);

    for (auto plane : m_planeWidgets)
        plane->SetLookupTable(lut);

    updateVisibilities();
}

void RenderedVectorGrid3D::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    updateVisibilities();
}

void RenderedVectorGrid3D::forceLICUpdate(int axis)
{
    if (!m_isInitialized || (m_colorMode != ColorMode::LIC))
        return;

    // missing update in vtkImageDataLIC2D?
    m_lic2D[axis]->Update();
}

void RenderedVectorGrid3D::resetSlicePositions()
{
    for (int i = 0; i < 3; ++i)
        m_planeWidgets[i]->SetSliceIndex(m_slicePositions[i]);
}

void RenderedVectorGrid3D::updateVisibilities()
{
    for (int i = 0; i < 3; ++i)
    {
        bool showSliceI = isVisible()
            && (colorMode() != ColorMode::UserDefined)
            && m_gradient // don't show the slice before they can use our gradient
            && (m_planeWidgets[i]->GetInteractor() != nullptr) // don't enable them without an interactor
            && m_slicesEnabled[i];

        m_planeWidgets[i]->SetEnabled(showSliceI);
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

int RenderedVectorGrid3D::slicePosition(int axis)
{
    assert(0 < axis || axis < 3);

    return m_slicePositions[axis];
}

void RenderedVectorGrid3D::setSlicePosition(int axis, int slicePosition)
{
    assert(0 < axis || axis < 3);

    m_slicePositions[axis] = slicePosition;

    m_planeWidgets[axis]->SetSliceIndex(slicePosition);

    forceLICUpdate(axis);
}

void RenderedVectorGrid3D::setLic2DVectorScaleFactor(float f)
{
    if (m_lic2DVectorScaleFactor == f)
        return;

    m_lic2DVectorScaleFactor = f;

    std::string fun{ std::to_string(m_lic2DVectorScaleFactor) + "*" + s_resliceOutputArray };

    for (auto vectorScale : m_lic2DVectorScale)
        vectorScale->SetFunction(fun.c_str());

    for (int i = 0; i < 3; ++i)
        forceLICUpdate(i);
}

RenderedVectorGrid3D::ColorMode RenderedVectorGrid3D::colorMode() const
{
    return m_colorMode;
}

void RenderedVectorGrid3D::setColorMode(ColorMode mode)
{
    m_colorMode = mode;

    for (int i = 0; i < 3; ++i)
    switch (m_colorMode)
    {
    case ColorMode::UserDefined:
        break;

    // slightly hacked: modify internal pipeline of vtkImagePlaneWidget to allow usage with LIC
    case ColorMode::ScalarMapping:
        m_planeWidgets[i]->GetTexture()->SetInputConnection(m_planeWidgets[i]->GetReslice()->GetOutputPort());
        break;
    case ColorMode::LIC:
        //m_planeWidgets[i]->GetTexture()->SetInputConnection(m_lic2DColorMagnitudeScalars[i]->GetOutputPort());
        m_planeWidgets[i]->GetTexture()->SetInputConnection(m_lic2D[i]->GetOutputPort());
        break;
    }
}
