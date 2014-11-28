#include "ImageProfileData.h"

#include <cmath>
#include <algorithm>

#include <vtkMath.h>

#include <vtkImageData.h>
#include <vtkCellData.h>
#include <vtkPointData.h>

#include <vtkLineSource.h>
#include <vtkProbeFilter.h>
#include <vtkTransformFilter.h>
#include <vtkTransform.h>
#include <vtkAssignAttribute.h>
#include <vtkWarpScalar.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/context2D_data/ImageProfileContextPlot.h>

#include <core/table_model/QVtkTableModelProfileData.h>


ImageProfileData::ImageProfileData(const QString & name, ImageDataObject * imageData)
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_imageData(imageData)
    , m_abscissa("position")
    , m_probeLine(vtkSmartPointer<vtkLineSource>::New())
    , m_transform(vtkSmartPointer<vtkTransformFilter>::New())
    , m_graphLine(vtkSmartPointer<vtkWarpScalar>::New())
{
    vtkDataArray * scalars = imageData->processedDataSet()->GetPointData()->GetScalars();
    if (!scalars)
        scalars = imageData->processedDataSet()->GetCellData()->GetScalars();
    assert(scalars);
    m_scalarsName = QString::fromLatin1(scalars->GetName());

    VTK_CREATE(vtkProbeFilter, probe);
    probe->SetInputConnection(m_probeLine->GetOutputPort());
    probe->SetSourceConnection(m_imageData->processedOutputPort());

    m_transform->SetInputConnection(probe->GetOutputPort());

    VTK_CREATE(vtkAssignAttribute, assign);
    assign->Assign(m_scalarsName.toLatin1().data(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    assign->SetInputConnection(m_transform->GetOutputPort());

    m_graphLine->UseNormalOn();
    m_graphLine->SetNormal(0, 1, 0);
    m_graphLine->SetInputConnection(assign->GetOutputPort());
}

bool ImageProfileData::is3D() const
{
    return false;
}

Context2DData * ImageProfileData::createContextData()
{
    return new ImageProfileContextPlot(this);
}

QString ImageProfileData::dataTypeName() const
{
    return "image profile";
}

vtkDataSet * ImageProfileData::processedDataSet()
{
    m_graphLine->Update();
    return m_graphLine->GetOutput();
}

vtkAlgorithmOutput * ImageProfileData::processedOutputPort()
{
    return m_graphLine->GetOutputPort();
}

const QString & ImageProfileData::abscissa() const
{
    return m_abscissa;
}

const QString & ImageProfileData::scalarsName() const
{
    return m_scalarsName;
}

const double * ImageProfileData::scalarRange()
{
    // x-y-plot -> value range on the y axis
    return &processedDataSet()->GetBounds()[2];
}

int ImageProfileData::numberOfScalars()
{
    return processedDataSet()->GetNumberOfPoints();
}

const double * ImageProfileData::point1() const
{
    return m_probeLine->GetPoint1();
}

const double * ImageProfileData::point2() const
{
    return m_probeLine->GetPoint2();
}

void ImageProfileData::setPoints(double point1[3], double point2[3])
{
    m_probeLine->SetPoint1(point1);
    m_probeLine->SetPoint2(point2);

    double probeVector[3];
    vtkMath::Subtract(point2, point1, probeVector);
    int numProbePoints = static_cast<int>(std::ceil(std::sqrt(vtkMath::Dot(probeVector, probeVector))));
    m_probeLine->SetResolution(numProbePoints);

    VTK_CREATE(vtkTransform, m);
    // move to origin
    double xTranslate = -point1[0];
    double yTranslate = -point1[1];
    // align to x axis
    double angle = vtkMath::DegreesFromRadians(std::atan2(probeVector[1], probeVector[0]));

    m->RotateZ(-angle);
    m->Translate(xTranslate, yTranslate, 0);
    m_transform->SetTransform(m);

    emit dataChanged();
    emit boundsChanged();
}

QVtkTableModel * ImageProfileData::createTableModel()
{
    QVtkTableModel * tableModel = new QVtkTableModelProfileData();
    tableModel->setDataObject(this);

    return tableModel;
}
