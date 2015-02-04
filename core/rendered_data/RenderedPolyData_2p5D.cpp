#include "RenderedPolyData_2p5D.h"

#include <cassert>
#include <cmath>

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
#include <vtkMath.h>

#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkImageDataToPointSet.h>
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
        const double earthR = 6378.138;
        double Fi0 = -25.167916666667;  // latitude 
        double La0 = -68.5062500000003; // longitude of the origin (~center) of the surface's local coordinate system


        // approximations for regions not larger than a few hundreds of kilometers:
        /*auto transformApprox = [earthR] (double Fi, double La, double Fi0, double La0, double & X, double & Y)
        {
            Y = earthR * (Fi - Fi0) * vtkMath::Pi() / 180;
            X = earthR * (La - La0) * std::cos(Fi0 / 180 * vtkMath::Pi()) * vtkMath::Pi() / 180;
        };*/

        double toLocalTranslation[3] = {
            -La0,
            -Fi0,
            0.0
        };
        double toLocalScale[3] = {
            earthR * std::cos(Fi0 / 180.0 * vtkMath::Pi()) * vtkMath::Pi() / 180.0,
            earthR * vtkMath::Pi() / 180.0,
            0  // flattening, elevation is stored in scalars
        };

        VTK_CREATE(vtkImageShiftScale, elevationToMeters);
        elevationToMeters->SetScale(0.001);
        elevationToMeters->SetInputData(m_demData->dataSet());

        // missing VTK filter to scale/translate images (along the grid orientation)? TODO that later
        VTK_CREATE(vtkImageDataToPointSet, toPoints);
        toPoints->SetInputConnection(elevationToMeters->GetOutputPort());

        VTK_CREATE(vtkWarpScalar, warpDEM);   // use point scalars as elevation
        warpDEM->SetInputConnection(toPoints->GetOutputPort());

        VTK_CREATE(vtkTransform, demTransform);
        demTransform->Scale(toLocalScale);
        demTransform->Translate(toLocalTranslation);

        VTK_CREATE(vtkTransformFilter, demTransformFilter);
        demTransformFilter->SetTransform(demTransform);
        demTransformFilter->SetInputConnection(warpDEM->GetOutputPort());

        VTK_CREATE(vtkTransform, meshTransform);
        meshTransform->Scale(5, 5, 0);  // flatten
        
        VTK_CREATE(vtkTransformFilter, meshTransformFilter);
        meshTransformFilter->SetTransform(meshTransform);
        meshTransformFilter->SetInputConnection(colorMappingOutput());

        VTK_CREATE(vtkProbeFilter, probe);
        probe->SetInputConnection(meshTransformFilter->GetOutputPort());
        probe->SetSourceConnection(demTransformFilter->GetOutputPort());

        VTK_CREATE(vtkWarpScalar, warp);
        warp->SetInputConnection(probe->GetOutputPort());

        mapper()->SetInputConnection(warp->GetOutputPort());
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
