#include "ImageProfileData.h"

#include <algorithm>
#include <cmath>

#include <vtkAssignAttribute.h>
#include <vtkBoundingBox.h>
#include <vtkCellData.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkWarpScalar.h>

#include <core/data_objects/ImageDataObject.h>
#include <core/context2D_data/ImageProfileContextPlot.h>

#include <core/table_model/QVtkTableModelProfileData.h>


namespace
{
vtkVector2d operator-(const vtkVector2d & lhs, const vtkVector2d & rhs)
{
    return vtkVector2d(lhs[0] - rhs[0], lhs[1] - rhs[1]);
}
}


ImageProfileData::ImageProfileData(
    const QString & name,
    DataObject & sourceData,
    const QString & scalarsName,
    IndexType scalarsLocation,
    vtkIdType vectorComponent)
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_isValid(false)
    , m_sourceData(sourceData)
    , m_abscissa("position")
    , m_scalarsName(scalarsName)
    , m_scalarsLocation(scalarsLocation)
    , m_vectorComponent(vectorComponent)
    , m_probeLine(vtkSmartPointer<vtkLineSource>::New())
    , m_transform(vtkSmartPointer<vtkTransformFilter>::New())
    , m_graphLine(vtkSmartPointer<vtkWarpScalar>::New())
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

    m_probe = vtkSmartPointer<vtkProbeFilter>::New();
    m_probe->SetInputConnection(m_probeLine->GetOutputPort());


    // for polygonal source data, flatten first
    if (vtkPolyData::SafeDownCast(m_sourceData.dataSet()))
    {
        auto flattenerTransform = vtkSmartPointer<vtkTransform>::New();
        flattenerTransform->Scale(1, 1, 0);
        auto flattener = vtkSmartPointer<vtkTransformFilter>::New();
        flattener->SetTransform(flattenerTransform);
        flattener->SetInputConnection(m_sourceData.processedOutputPort());

        m_probe->SetSourceConnection(flattener->GetOutputPort());
    }
    else
    {
        m_probe->SetSourceConnection(m_sourceData.processedOutputPort());
    }

    m_transform->SetInputConnection(m_probe->GetOutputPort());

    auto assign = vtkSmartPointer<vtkAssignAttribute>::New();
    auto assignLocation = m_scalarsLocation == IndexType::points ? vtkAssignAttribute::POINT_DATA : vtkAssignAttribute::CELL_DATA;
    assign->Assign(m_scalarsName.toUtf8().data(), vtkDataSetAttributes::SCALARS, assignLocation);
    assign->SetInputConnection(m_transform->GetOutputPort());

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

vtkDataSet * ImageProfileData::probedLine()
{
    m_probe->Update();
    return m_probe->GetOutput();
}

vtkAlgorithmOutput * ImageProfileData::probedLineOuputPort()
{
    return m_probe->GetOutputPort();
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

    m_probeLine->SetPoint1(point1[0], point1[1], 0.0);
    m_probeLine->SetPoint2(point2[0], point2[1], 0.0);
    
    auto probeVector = point2 - point1;

    int numProbePoints = 0;
    double pointSpacing[3];
    if (auto image = vtkImageData::SafeDownCast(m_sourceData.dataSet()))
    {
        image->GetSpacing(pointSpacing);
        double xLength = probeVector[0] / pointSpacing[0];
        double yLength = probeVector[1] / pointSpacing[1];
        numProbePoints = static_cast<int>(std::sqrt(xLength * xLength + yLength * yLength));
    }
    else
    {
        auto vectorLength = probeVector.Norm();
        double bounds[6];
        m_sourceData.dataSet()->GetBounds(bounds);

        vtkBoundingBox bbox(bounds);
        auto diagonalLength = bbox.GetDiagonalLength();

        vtkIdType numElements = 0;

        if (m_scalarsLocation == IndexType::cells)
        {
            numElements = m_sourceData.dataSet()->GetNumberOfCells();
        }
        else
        {
            numElements = m_sourceData.dataSet()->GetNumberOfPoints();
        }

        numElements = static_cast<vtkIdType>(std::sqrt(numElements));

        // assuming that the points/cells are somehow uniformly distributed (and sized)
        numProbePoints = static_cast<int>(std::ceil(double(numElements) * vectorLength / diagonalLength));
        numProbePoints = static_cast<int>(std::max(1, numProbePoints));
    }

    m_probeLine->SetResolution(numProbePoints);

    auto m = vtkSmartPointer<vtkTransform>::New();
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

std::unique_ptr<QVtkTableModel> ImageProfileData::createTableModel()
{
    std::unique_ptr<QVtkTableModel> tableModel = std::make_unique<QVtkTableModelProfileData>();
    tableModel->setDataObject(this);

    return tableModel;
}
