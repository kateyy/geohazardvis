#include "ImageProfileData.h"

#include <algorithm>
#include <cmath>

#include <vtkAssignAttribute.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWarpScalar.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/context2D_data/ImageProfileContextPlot.h>
#include <core/filters/LinearSelectorXY.h>
#include <core/table_model/QVtkTableModelProfileData.h>
#include <core/utility/vtkvectorhelper.h>


ImageProfileData::ImageProfileData(
    const QString & name,
    DataObject & sourceData,
    const QString & scalarsName,
    IndexType scalarsLocation,
    vtkIdType vectorComponent)
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_isValid{ false }
    , m_sourceData{ sourceData }
    , m_abscissa{ "position" }
    , m_scalarsName{ scalarsName }
    , m_scalarsLocation{ scalarsLocation }
    , m_vectorComponent{ vectorComponent }
    , m_inputIsImage{ dynamic_cast<ImageDataObject *>(&sourceData) != nullptr }
    , m_outputTransformation{ vtkSmartPointer<vtkTransformPolyDataFilter>::New() }
    , m_graphLine{ vtkSmartPointer<vtkWarpScalar>::New() }
{
    auto inputData = sourceData.processedDataSet();

    auto attributeData = (scalarsLocation == IndexType::points)
        ? static_cast<vtkDataSetAttributes *>(inputData->GetPointData())
        : static_cast<vtkDataSetAttributes *>(inputData->GetCellData());

    auto scalars = attributeData->GetArray(scalarsName.toUtf8().data());

    if (scalars == nullptr || scalars->GetNumberOfComponents() < vectorComponent)
    {
        return;
    }


    if (m_inputIsImage)
    {
        m_probeLine = vtkSmartPointer<vtkLineSource>::New();
        m_imageProbe = vtkSmartPointer<vtkProbeFilter>::New();
        m_imageProbe->SetInputConnection(m_probeLine->GetOutputPort());
        m_imageProbe->SetSourceConnection(sourceData.processedOutputPort());

        m_outputTransformation->SetInputConnection(m_imageProbe->GetOutputPort());
    }
    else
    {
        auto & poly = dynamic_cast<PolyDataObject &>(sourceData);
        m_polyDataPointsSelector = vtkSmartPointer<LinearSelectorXY>::New();
        m_polyDataPointsSelector->SetInputConnection(sourceData.processedOutputPort());
        m_polyDataPointsSelector->SetCellCentersConnection(poly.cellCentersOutputPort());

        m_outputTransformation->SetInputConnection(m_polyDataPointsSelector->GetOutputPort(1));
    }

    auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
    // output geometry is always points
    assign->Assign(m_scalarsName.toUtf8().data(), vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
    assign->SetInputConnection(m_outputTransformation->GetOutputPort());

    m_graphLine->UseNormalOn();
    m_graphLine->SetNormal(0, 1, 0);
    m_graphLine->SetInputConnection(assign->GetOutputPort());

    m_isValid = true;
}

bool ImageProfileData::isValid() const
{
    return m_isValid;
}

bool ImageProfileData::is3D() const
{
    return false;
}

std::unique_ptr<Context2DData> ImageProfileData::createContextData()
{
    return std::make_unique<ImageProfileContextPlot>(*this);
}

const QString & ImageProfileData::dataTypeName() const
{
    return dataTypeName_s();
}

const QString & ImageProfileData::dataTypeName_s()
{
    static const QString name{ "image profile" };
    return name;
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

const DataObject & ImageProfileData::sourceData() const
{
    return m_sourceData;
}

const QString & ImageProfileData::scalarsName() const
{
    return m_scalarsName;
}

IndexType ImageProfileData::scalarsLocation() const
{
    return m_scalarsLocation;
}

vtkIdType ImageProfileData::vectorComponent() const
{
    return m_vectorComponent;
}

const double * ImageProfileData::scalarRange()
{
    // x-y-plot -> value range on the y axis
    return &processedDataSet()->GetBounds()[2];
}

int ImageProfileData::numberOfScalars()
{
    return static_cast<int>(processedDataSet()->GetNumberOfPoints());
}

const vtkVector2d & ImageProfileData::point1() const
{
    return m_point1;
}

const vtkVector2d & ImageProfileData::point2() const
{
    return m_point2;
}

void ImageProfileData::setPoints(const vtkVector2d & point1, const vtkVector2d & point2)
{
    m_point1 = point1;
    m_point2 = point2;
    const auto probeVector = point2 - point1;

    if (auto image = vtkImageData::SafeDownCast(m_sourceData.dataSet()))
    {
        m_probeLine->SetPoint1(point1[0], point1[1], 0.0);
        m_probeLine->SetPoint2(point2[0], point2[1], 0.0);


        double pointSpacing[3];
        image->GetSpacing(pointSpacing);
        double xLength = probeVector[0] / pointSpacing[0];
        double yLength = probeVector[1] / pointSpacing[1];
        int numProbePoints = static_cast<int>(std::sqrt(xLength * xLength + yLength * yLength));

        m_probeLine->SetResolution(numProbePoints);
    }
    else
    {
        m_polyDataPointsSelector->SetStartPoint(m_point1);
        m_polyDataPointsSelector->SetEndPoint(m_point2);
    }


    auto m = vtkSmartPointer<vtkTransform>::New();
    m->PostMultiply();

    // move to origin
    m->Translate(-point1[0], -point1[1], 0);
    // align to x axis
    m->RotateZ(
        -vtkMath::DegreesFromRadians(std::atan2(probeVector[1], probeVector[0])));

    m_outputTransformation->SetTransform(m);

    emit dataChanged();
    emit boundsChanged();
}

std::unique_ptr<QVtkTableModel> ImageProfileData::createTableModel()
{
    std::unique_ptr<QVtkTableModel> tableModel = std::make_unique<QVtkTableModelProfileData>();
    tableModel->setDataObject(this);

    return tableModel;
}
