#include "RenderedVectorGrid3D.h"

#include <algorithm>
#include <limits>

#include <vtkInformation.h>
#include <vtkInformationStringKey.h>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <vtkAssignAttribute.h>
#include <vtkArrayCalculator.h>
#include <vtkExtractVOI.h>
#include <vtkImageDataLIC2D.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLExtensionManager.h>

#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageProperty.h>

#include <vtkProp3DCollection.h>
#include <vtkLookupTable.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/filters/NoiseImageSource.h>
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
    , m_isInitialized(false)
    , m_extractVOI(vtkSmartPointer<vtkExtractVOI>::New())
    , m_nullImage(vtkSmartPointer<vtkImageData>::New())
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

    std::string vectorsName{ image->GetPointData()->GetVectors()->GetName() };
    std::string vectorsScaledName{ vectorsName + "_scaled" };

    m_lic2DVectorScale = vtkSmartPointer<vtkArrayCalculator>::New();
    m_lic2DVectorScale->SetInputData(image);
    m_lic2DVectorScale->AddVectorArrayName(vectorsName.c_str());
    m_lic2DVectorScale->SetResultArrayName(vectorsScaledName.c_str());

    m_glContext = vtkSmartPointer<vtkRenderWindow>::New();
    vtkOpenGLRenderWindow * openGLContext = vtkOpenGLRenderWindow::SafeDownCast(m_glContext);
    assert(openGLContext);
    openGLContext->GetExtensionManager()->IgnoreDriverBugsOn();
    openGLContext->OffScreenRenderingOn();


    double scalarRange[2] = { std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest() };
    for (int i = 0; i < 3; ++i)
    {
        double r[2];
        image->GetPointData()->GetVectors()->GetRange(r, i);
        scalarRange[0] = std::min(scalarRange[0], r[0]);
        scalarRange[1] = std::max(scalarRange[1], r[1]);
    }
    setLic2DVectorScaleFactor(1.0 / std::max(std::abs(scalarRange[0]), std::abs(scalarRange[1])));

    VTK_CREATE(vtkAssignAttribute, assignScaledVectors);
    assignScaledVectors->SetInputConnection(m_lic2DVectorScale->GetOutputPort());
    assignScaledVectors->Assign(vectorsScaledName.c_str(),
        vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    m_imgProperty = vtkSmartPointer<vtkImageProperty>::New();
    m_imgProperty->UseLookupTableScalarRangeOn();
    m_imgProperty->SetDiffuse(0.8);
    m_imgProperty->SetAmbient(0.5);

    VTK_CREATE(NoiseImageSource, noise);
    noise->SetExtent(0, 1270, 0, 1270, 0, 0);
    noise->SetValueRange(0, 1);
    noise->SetNumberOfComponents(2);

    for (int i = 0; i < 3; ++i)
    {
        /** Line Integral Convolution 2D */

        m_lic2DVOI[i] = vtkSmartPointer<vtkExtractVOI>::New();
        m_lic2DVOI[i]->SetInputConnection(assignScaledVectors->GetOutputPort());

        m_lic2D[i] = vtkSmartPointer<vtkImageDataLIC2D>::New();
        m_lic2D[i]->SetInputConnection(0, m_lic2DVOI[i]->GetOutputPort());
        m_lic2D[i]->SetInputConnection(1, noise->GetOutputPort());
        m_lic2D[i]->SetSteps(50);
        m_lic2D[i]->GlobalWarningDisplayOff();
        m_lic2D[i]->SetContext(m_glContext);


        /** image rendering (LIC/scalars) */
        m_sliceMappers[i] = vtkSmartPointer<vtkImageSliceMapper>::New();
        m_sliceMappers[i]->SetOrientation(i);  // 0, 1, 2 maps to X, Y, Z

        m_slices[i] = vtkSmartPointer<vtkImageSlice>::New();
        m_slices[i]->SetMapper(m_sliceMappers[i]);
        m_slices[i]->SetProperty(m_imgProperty);

        int min = extent[2 * i];
        int max = extent[2 * i + 1];
        setSlicePosition(i, (max - min) / 2);

        m_slicesEnabled[i] = true;
    }


    setColorMode(ColorMode::UserDefined);

    m_isInitialized = true;
}

RenderedVectorGrid3D::~RenderedVectorGrid3D()
{
    for (auto & lic : m_lic2D)
        lic->SetContext(nullptr);
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
                [this, i] () { return m_slicePositions[i]; },
                [this, i] (int value) {
                setSlicePosition(i, value);
                emit geometryChanged();
            });
            slice_prop->setOption("title", axis);
            slice_prop->setOption("minimum", extent[2 * i]);
            slice_prop->setOption("maximum", extent[2 * i + 1]);
        }

        vtkImageProperty * property = m_imgProperty;

        auto prop_transparency = group_scalarSlices->addProperty<double>("Transparency",
            [property]() {
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

    auto group_LIC2D = renderSettings->addGroup("LIC2D");
    group_LIC2D->setOption("title", "Line Integral Convolution 2D");
    {
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
    ColorMode newMode = ColorMode::UserDefined;
    if (m_scalars)
    {
        if (m_scalars->scalarsName() == QString::fromUtf8(
            dataObject()->dataSet()->GetPointData()->GetVectors()->GetName()))
            newMode = ColorMode::ScalarMapping;
        else if (m_scalars->name() == "LIC 2D")
            newMode = ColorMode::LIC;
    }

    setColorMode(newMode);

    updateVisibilities();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    m_imgProperty->SetLookupTable(m_gradient);
}

void RenderedVectorGrid3D::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    updateVisibilities();
}

void RenderedVectorGrid3D::forceLICUpdate(int axis)
{
    if (!m_isInitialized)
        return;

    // missing update in vtkImageDataLIC2D?
    m_lic2D[axis]->Update();
}

void RenderedVectorGrid3D::updateVisibilities()
{
    for (int i = 0; i < 3; ++i)
    {
        bool showSliceI = (colorMode() != ColorMode::UserDefined) && m_slicesEnabled[i];
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

    m_slicePositions[axis] = slicePosition;

    m_sliceMappers[axis]->SetSliceNumber(slicePosition);

    int voi[6];
    static_cast<vtkImageData *>(dataObject()->dataSet())->GetExtent(voi);
    voi[2 * axis] = voi[2 * axis + 1] = slicePosition;
    m_lic2DVOI[axis]->SetVOI(voi);

    forceLICUpdate(axis);
}

void RenderedVectorGrid3D::setLic2DVectorScaleFactor(float f)
{
    if (m_lic2DVectorScaleFactor == f)
        return;

    m_lic2DVectorScaleFactor = f;

    std::string vectorsName = dataObject()->dataSet()->GetPointData()->GetVectors()->GetName();

    std::string fun{ std::to_string(m_lic2DVectorScaleFactor) 
                     + "*" + vectorsName };
    m_lic2DVectorScale->SetFunction(fun.c_str());

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
        m_sliceMappers[i]->SetInputData(m_nullImage);
        break;
    case ColorMode::ScalarMapping:
        m_sliceMappers[i]->SetInputConnection(dataObject()->processedOutputPort());
        break;
    case ColorMode::LIC:
        m_sliceMappers[i]->SetInputConnection(m_lic2D[i]->GetOutputPort());
        break;
    }
}
