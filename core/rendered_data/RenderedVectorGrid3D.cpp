#include "RenderedVectorGrid3D.h"

#include <vtkProp3DCollection.h>
#include <vtkLookupTable.h>

#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

#include <vtkAssignAttribute.h>
#include <vtkExtractVOI.h>
#include <vtkCellPicker.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReslice.h>
#include <vtkInformation.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTexture.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>
#include <core/filters/ArrayRenameFilter.h>
#include <core/filters/ImagePlaneWidget.h>


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

const std::string s_resliceOutputArray = "ImageScalars";
const std::string s_lic2DWithMangnitudes = "LIC2DWithMagnitudes";

}

RenderedVectorGrid3D::RenderedVectorGrid3D(VectorGrid3DDataObject & dataObject)
    : RenderedData3D(dataObject)
    , m_isInitialized(false)
    , m_extractVOI(vtkSmartPointer<vtkExtractVOI>::New())
    , m_slicesEnabled()
{
    vtkImageData * image = static_cast<vtkImageData *>(dataObject.processedDataSet());
    assert(image);

    int extent[6];
    image->GetExtent(extent);

    m_extractVOI->SetVOI(extent);
    m_extractVOI->SetInputData(dataObject.dataSet());

    // decrease sample rate to prevent crashing/blocking rendering
    vtkIdType numPoints = image->GetNumberOfPoints();
    int sampleRate = std::max(1, (int)std::floor(std::cbrt(float(numPoints) / DefaultMaxNumberOfPoints)));
    setSampleRate(sampleRate, sampleRate, sampleRate);


    auto planePicker = vtkSmartPointer<vtkCellPicker>::New(); // shared picker for the plane widgets

    for (int i = 0; i < 3; ++i)
    {
        /** Reslice widgets  */

        m_planeWidgets[i] = vtkSmartPointer<ImagePlaneWidget>::New();
        // TODO VTK Bug? A pipeline connection does not work for some reason
        //m_planeWidgets[i]->SetInputConnection(dataObject->processedOutputPort());
        m_planeWidgets[i]->SetInputData(dataObject.dataSet());
        m_planeWidgets[i]->UserControlledLookupTableOn();
        m_planeWidgets[i]->RestrictPlaneToVolumeOn();
        m_planeWidgets[i]->SetPlaneOrientation(i);

        // this is required to fix picking with multiple planes in a view
        m_planeWidgets[i]->SetPicker(planePicker);
        // this is recommended for rendering with other transparent objects
        m_planeWidgets[i]->GetColorMap()->SetOutputFormatToRGBA();
        m_planeWidgets[i]->GetColorMap()->PassAlphaToOutputOn();

        m_planeWidgets[i]->SetLeftButtonAction(vtkImagePlaneWidget::VTK_SLICE_MOTION_ACTION);
        m_planeWidgets[i]->SetRightButtonAction(vtkImagePlaneWidget::VTK_CURSOR_ACTION);

        auto & mapperInfo = *m_planeWidgets[i]->GetTexturePlaneMapper()->GetInformation();

        DataObject::storePointer(mapperInfo, &dataObject);
        mapperInfo.Set(DataObject::NameKey(), dataObject.name().toUtf8().data());
        AbstractVisualizedData::storePointer(mapperInfo, this);

        auto property = vtkSmartPointer<vtkProperty>::New();
        property->LightingOn();
        property->SetInterpolationToPhong();
        m_planeWidgets[i]->SetTexturePlaneProperty(property);


        int min = extent[2 * i];
        int max = extent[2 * i + 1];
        setSlicePosition(i, (max - min) / 2);

        m_slicesEnabled[i] = true;
    }

    m_isInitialized = true;
}

void RenderedVectorGrid3D::setRenderWindowInteractor(vtkRenderWindowInteractor * interactor)
{
    for (auto widget : m_planeWidgets)
        widget->SetInteractor(interactor);

    updateVisibilities();
}

VectorGrid3DDataObject & RenderedVectorGrid3D::vectorGrid3DDataObject()
{
    return static_cast<VectorGrid3DDataObject &>(dataObject());
}

const VectorGrid3DDataObject & RenderedVectorGrid3D::vectorGrid3DDataObject() const
{
    return static_cast<const VectorGrid3DDataObject &>(dataObject());
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

        for (int i = 0; i < 3; ++i)
            prop_visibilities->asCollection()->at(i)->setOption("title", std::string{ char('X' + i) });


        int extent[6];
        static_cast<vtkImageData *>(dataObject().dataSet())->GetExtent(extent);

        auto prop_positions = group_scalarSlices->addGroup("Positions");

        for (int i = 0; i < 3; ++i)
        {
            std::string axis = { char('X' + i) };

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

        std::string depthWarning{ "Using transparency and lighting at the same time probably leads to rendering errors." };

        auto prop_transparencies = group_scalarSlices->addGroup("Transparencies");

        for (int i = 0; i < 3; ++i)
        {
            std::string axis = { char('X' + i) };

            vtkProperty * property = m_planeWidgets[i]->GetTexturePlaneProperty();

            auto * slice_prop = prop_transparencies->addProperty<double>(axis + "slice",
                [property]() {
                return (1.0 - property->GetOpacity()) * 100;
            },
                [this, property] (double transparency) {
                property->SetOpacity(1.0 - transparency * 0.01);
                emit geometryChanged();
            });
            slice_prop->setOption("title", axis);
            slice_prop->setOption("minimum", 0);
            slice_prop->setOption("maximum", 100);
            slice_prop->setOption("step", 1);
            slice_prop->setOption("suffix", " %");
            slice_prop->setOption("tooltip", depthWarning);
        }

        vtkProperty * property = m_planeWidgets[0]->GetTexturePlaneProperty();

        auto prop_lighting = group_scalarSlices->addProperty<bool>("Lighting",
            [property]() { return property->GetLighting(); },
            [this, property] (bool lighting) {
            for (auto w : m_planeWidgets)
                w->GetTexturePlaneProperty()->SetLighting(lighting);
            emit geometryChanged();
        });
        prop_lighting->setOption("tooltip", depthWarning);

        auto prop_diffLighting = group_scalarSlices->addProperty<double>("DiffuseLighting",
            [property] () { return property->GetDiffuse(); },
            [this, property](double diff) {
            for (auto w : m_planeWidgets)
                w->GetTexturePlaneProperty()->SetDiffuse(diff);
            emit geometryChanged();
        });
        prop_diffLighting->setOption("title", "Diffuse Lighting");
        prop_diffLighting->setOption("minimum", 0);
        prop_diffLighting->setOption("maximum", 1);
        prop_diffLighting->setOption("step", 0.05);

        auto prop_ambientLighting = group_scalarSlices->addProperty<double>("AmbientLighting",
            [property]() { return property->GetAmbient(); },
            [this, property] (double ambient) {
            for (auto w : m_planeWidgets)
                w->GetTexturePlaneProperty()->SetAmbient(ambient);
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

    for (int i = 0; i < 3; ++i)
        if (m_planeWidgets[i]->GetEnabled() != 0)
            m_storedSliceIndexes[i] = m_planeWidgets[i]->GetSliceIndex();

    if (m_colorMappingData && m_colorMappingData->usesFilter())
    {
        for (int i = 0; i < 3; ++i)
        {
            m_planeWidgets[i]->GetTexture()->SetInputConnection(
                m_colorMappingData->createFilter(this, i)->GetOutputPort());
        }
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            m_planeWidgets[i]->GetTexture()->SetInputConnection(
                m_planeWidgets[i]->GetReslice()->GetOutputPort());
        }
    }

    for (int i = 0; i < 3; ++i)
        m_planeWidgets[i]->SetSliceIndex(m_storedSliceIndexes[i]);


    updateVisibilities();

    updatePlaneLUT();
}

void RenderedVectorGrid3D::colorMappingGradientChangedEvent()
{
    RenderedData3D::colorMappingGradientChangedEvent();

    updatePlaneLUT();

    updateVisibilities();
}

void RenderedVectorGrid3D::visibilityChangedEvent(bool visible)
{
    RenderedData3D::visibilityChangedEvent(visible);

    updateVisibilities();
}

void RenderedVectorGrid3D::updatePlaneLUT()
{
    if (!m_blackWhiteLUT)
    {
        m_blackWhiteLUT = vtkSmartPointer<vtkLookupTable>::New();
        m_blackWhiteLUT->SetHueRange(0, 0);
        m_blackWhiteLUT->SetSaturationRange(0, 0);
        m_blackWhiteLUT->SetValueRange(0, 1);
        m_blackWhiteLUT->SetNumberOfTableValues(255);
        m_blackWhiteLUT->Build();
    }

    vtkLookupTable * lut = (m_colorMappingData && m_colorMappingData->name() == "LIC 2D")
        ? m_blackWhiteLUT.Get()
        : vtkLookupTable::SafeDownCast(m_gradient);

    for (auto plane : m_planeWidgets)
        plane->SetLookupTable(lut);
}

void RenderedVectorGrid3D::updateVisibilities()
{
    bool colorMapping = m_colorMappingData && m_colorMappingData->usesFilter();
    bool changed = false;

    for (int i = 0; i < 3; ++i)
    {
        bool showSliceI = isVisible()
            && colorMapping
            && m_gradient // don't show the slice before they can use our gradient
            && (m_planeWidgets[i]->GetInteractor() != nullptr) // don't enable them without an interactor
            && m_slicesEnabled[i];

        if ((m_planeWidgets[i]->GetEnabled() != 0) != showSliceI)   // prevent interactor not set warning
        {
            m_planeWidgets[i]->SetEnabled(showSliceI);
            changed = true;
        }
    }

    if (changed)
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
    assert(0 <= axis && axis < 3);

    return m_planeWidgets[axis]->GetSliceIndex();
}

void RenderedVectorGrid3D::setSlicePosition(int axis, int slicePosition)
{
    assert(0 <= axis && axis < 3);

    m_storedSliceIndexes[axis] = slicePosition;
    m_planeWidgets[axis]->SetSliceIndex(slicePosition);
}

int RenderedVectorGrid3D::numberOfColorMappingInputs() const
{
    return 3;
}

vtkAlgorithmOutput * RenderedVectorGrid3D::colorMappingInput(int connection)
{
    assert(connection >= 0 && connection < 3);

    auto & filter = m_colorMappingInputs.at(connection);

    if (!filter)
    {
        bool hasVectors = dataObject().dataSet()->GetPointData()->GetVectors() != nullptr;

        auto rename = vtkSmartPointer<ArrayRenameFilter>::New();
        rename->SetScalarsName(dataObject().dataSet()->GetPointData()->GetScalars()->GetName());
        rename->SetInputConnection(m_planeWidgets[connection]->GetReslice()->GetOutputPort());
        filter = rename;

        if (hasVectors)
        {
            auto assignVectors = vtkSmartPointer<vtkAssignAttribute>::New();
            assignVectors->Assign(vtkDataSetAttributes::SCALARS, vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);
            assignVectors->SetInputConnection(rename->GetOutputPort());
            filter = assignVectors;
        }

    }

    return filter->GetOutputPort();
}
