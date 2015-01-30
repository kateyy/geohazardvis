#include "RenderedPolyData_2p5D.h"

#include <cassert>

#include <QDialogButtonBox>
#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QScopedPointer>

#include <vtkInformation.h>
#include <vtkInformationIntegerPointerKey.h>
#include <vtkInformationStringKey.h>

#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProbeFilter.h>
#include <vtkScalarsToColors.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>

#include <vtkProperty.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>

#include <reflectionzeug/PropertyGroup.h>

#include <core/vtkhelper.h>
#include <core/DataSetHandler.h>
#include <core/data_objects/ImageDataObject.h>


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

bool selectDEMDialog(ImageDataObject * & DEMObject)
{
    QList<ImageDataObject *> images;

    const auto & dataSets = DataSetHandler::instance().dataSets();
    for (DataObject * dataObject : dataSets)
    {
        if (auto image = dynamic_cast<ImageDataObject *>(dataObject))
        {
            if (image->imageData()->GetNumberOfScalarComponents() == 1)
                images << image;
        }
    }

    QDialog dialog(nullptr, Qt::Dialog);
    QLayout * layout = new QGridLayout();
    dialog.setLayout(layout);
    QLabel * label = new QLabel("Select a data set to be applied as Digital Elevation Model.");
    layout->addWidget(label);
    QComboBox * combo = new QComboBox();
    layout->addWidget(combo);
    QDialogButtonBox * buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Abort);
    layout->addWidget(buttons);

    dialog.connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    dialog.connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    for (auto image : images)
    {
        combo->addItem(image->name(), reinterpret_cast<size_t>(image));
    }

    if (dialog.exec() != QDialog::Accepted)
        return false;

    auto selection = combo->currentData();
    if (!selection.isValid())
        return false;

    bool ok;
    DEMObject = reinterpret_cast<ImageDataObject *>(selection.toULongLong(&ok));

    return ok;
}

}

RenderedPolyData_2p5D::RenderedPolyData_2p5D(PolyDataObject * dataObject)
    : RenderedPolyData(dataObject)
    , m_applyDEM(false)
{
}

reflectionzeug::PropertyGroup * RenderedPolyData_2p5D::createConfigGroup()
{
    auto settings = new PropertyGroup();

    auto prop_DEM = settings->addProperty<bool>("ApplyDEM",
        [this] () { return m_applyDEM; },
        [this] (bool apply) { setApplyDEM(apply); });
    prop_DEM->setOption("title", "Apply DEM");

    QScopedPointer<PropertyGroup> polyDataSettings{ RenderedPolyData::createConfigGroup() };
    polyDataSettings->forEach([settings] (AbstractProperty & prop){
        settings->addProperty(&prop);
    });
    polyDataSettings->setOwnsProperties(false);

    return settings;
}

vtkProperty * RenderedPolyData_2p5D::createDefaultRenderProperty() const
{
    vtkProperty * prop = RenderedPolyData::createDefaultRenderProperty();
    prop->SetColor(0, 0.6, 0);

    return prop;
}

void RenderedPolyData_2p5D::finalizePipeline()
{
    if (m_applyDEM)
    {
        double scale[3];
        double demBounds[3], dataBounds[3];
        scale[0] = (m_demData->bounds()[1] - m_demData->bounds()[0]) / (dataObject()->bounds()[1] - dataObject()->bounds()[0]);
        scale[1] = (m_demData->bounds()[3] - m_demData->bounds()[2]) / (dataObject()->bounds()[3] - dataObject()->bounds()[2]);
        scale[2] = 0;
        double translation[3];
        translation[0] = (m_demData->bounds()[1] + m_demData->bounds()[0]) / 2;
        translation[1] = (m_demData->bounds()[3] + m_demData->bounds()[2]) / 2;
        translation[2] = 0;


        VTK_CREATE(vtkTransform, surfaceTransform);
        surfaceTransform->Translate(translation);
        surfaceTransform->Scale(scale);

        VTK_CREATE(vtkTransformFilter, surfaceTransformFilter);
        surfaceTransformFilter->SetTransform(surfaceTransform);
        surfaceTransformFilter->SetInputConnection(colorMappingOutput());

        VTK_CREATE(vtkImageShiftScale, imageTransform);
        imageTransform->SetInputConnection(m_demData->processedOutputPort());
        imageTransform->SetScale(0.0001); // m to km

        VTK_CREATE(vtkProbeFilter, probe);
        probe->SetInputConnection(surfaceTransformFilter->GetOutputPort());
        probe->SetSourceConnection(imageTransform->GetOutputPort());

        VTK_CREATE(vtkWarpScalar, warp);
        warp->SetInputConnection(probe->GetOutputPort());

        VTK_CREATE(vtkTransform, reverseTransform);
        reverseTransform->Scale(1 / scale[0], 1 / scale[1], 1);
        reverseTransform->Translate(-translation[0], -translation[1], -translation[2]);

        VTK_CREATE(vtkTransformFilter, reverseTransformFilter);
        reverseTransformFilter->SetTransform(reverseTransform);
        reverseTransformFilter->SetInputConnection(warp->GetOutputPort());

        mapper()->SetInputConnection(reverseTransformFilter->GetOutputPort());
    }
    else
    {
        RenderedPolyData::finalizePipeline();
    }
}

void RenderedPolyData_2p5D::setApplyDEM(bool apply)
{
    if (!apply)
    {
        m_applyDEM = false;
        m_demData = nullptr;
        finalizePipeline();
        return;
    }

    ImageDataObject * dem;
    if (selectDEMDialog(dem))
    {
        m_applyDEM = true;
        m_demData = dem;
        finalizePipeline();
    }
}
