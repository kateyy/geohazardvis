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


ImageProfileData::ImageProfileData(const QString & name, DataObject & sourceData, const QString & scalarsName)
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_sourceData(sourceData)
    , m_abscissa("position")
    , m_probeLine(vtkSmartPointer<vtkLineSource>::New())
    , m_transform(vtkSmartPointer<vtkTransformFilter>::New())
    , m_graphLine(vtkSmartPointer<vtkWarpScalar>::New())
{
    auto inputData = sourceData.processedDataSet();

    vtkDataArray * scalars = inputData->GetPointData()->GetArray(scalarsName.toUtf8().data());
    m_scalarsLocation = vtkAssignAttribute::POINT_DATA;

    if (!scalars)
    {
        if (scalars = inputData->GetCellData()->GetArray(scalarsName.toUtf8().data()))
            m_scalarsLocation = vtkAssignAttribute::CELL_DATA;
    }

    // fallback: use data set scalars
    if (!scalars)
        scalars = inputData->GetPointData()->GetScalars();
    if (!scalars)
    {
        if (scalars = inputData->GetCellData()->GetArray(scalarsName.toUtf8().data()))
            m_scalarsLocation = vtkAssignAttribute::CELL_DATA;
    }

    m_isValid = scalars != nullptr;
    if (!m_isValid)
        return;

    m_scalarsName = QString::fromUtf8(scalars->GetName());

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
    assign->Assign(m_scalarsName.toUtf8().data(), vtkDataSetAttributes::SCALARS, m_scalarsLocation);
    assign->SetInputConnection(m_transform->GetOutputPort());

    m_graphLine->UseNormalOn();
    m_graphLine->SetNormal(0, 1, 0);
    m_graphLine->SetInputConnection(assign->GetOutputPort());
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
    return std::make_unique<ImageProfileContextPlot>(*this, m_scalarsName);
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
    return static_cast<int>(processedDataSet()->GetNumberOfPoints());
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
        auto vectorLength = vtkMath::Norm(probeVector);
        double bounds[6];
        m_sourceData.dataSet()->GetBounds(bounds);

        vtkBoundingBox bbox(bounds);
        auto diagonalLength = bbox.GetDiagonalLength();

        int numElements = 0;

        if (m_scalarsLocation == vtkAssignAttribute::CELL_DATA)
        {
            numElements = m_sourceData.dataSet()->GetNumberOfCells();
        }
        else
        {
            numElements = m_sourceData.dataSet()->GetNumberOfPoints();
        }

        // assuming that the points/cells are somehow uniformly distributed (and sized)
        numProbePoints = static_cast<int>(numElements * vectorLength / diagonalLength);
        numProbePoints = std::max(10, numProbePoints);
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
