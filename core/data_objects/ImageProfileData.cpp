#include "ImageProfileData.h"

#include <cmath>

#include <vtkMath.h>

#include <vtkLineSource.h>

#include <vtkImageData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>

#include <vtkProbeFilter.h>

#include <core/vtkhelper.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/ImageProfilePlot.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/PolyDataObject.h>


ImageProfileData::ImageProfileData(const QString & name, ImageDataObject * imageData, double point1[3], double point2[3])
    : DataObject(name, vtkSmartPointer<vtkImageData>::New())
    , m_imageData(imageData)
    , m_graphLine()
{
    vtkDataArray * scalars = imageData->processedDataSet()->GetCellData()->GetScalars();
    if (!scalars)
        scalars = imageData->processedDataSet()->GetPointData()->GetScalars();
    assert(scalars);
    m_scalarsName = QString::fromLatin1(scalars->GetName());

    for (int i = 0; i < 3; ++i)
    {
        m_point1[i] = point1[i];
        m_point2[i] = point2[i];
    }

    VTK_CREATE(vtkLineSource, probeLine);
    probeLine->SetPoint1(m_point1);
    probeLine->SetPoint2(m_point2);
    double probeVector[3];
    vtkMath::Subtract(m_point1, m_point2, probeVector);
    int numProbePoints = static_cast<int>(std::ceil(std::sqrt(vtkMath::Dot(probeVector, probeVector))));
    probeLine->SetResolution(numProbePoints);

    VTK_CREATE(vtkProbeFilter, probe);
    probe->SetInputConnection(probeLine->GetOutputPort());
    probe->SetSourceConnection(m_imageData->processedOutputPort());

    probe->Update();
    vtkPolyData * probedLine = probe->GetPolyDataOutput();
    vtkDataArray * probedScalars = probedLine->GetPointData()->GetArray(m_scalarsName.toLatin1().data());
    assert(probedScalars);

    vtkIdType numPoints = probedLine->GetNumberOfPoints();

    VTK_CREATE(vtkPoints, graphPoints);
    graphPoints->SetNumberOfPoints(numPoints);

    for (vtkIdType i = 0; i < numPoints; ++i)
    {
        double value = probedScalars->GetTuple(i)[0];
        graphPoints->SetPoint(i,
            i,
            value,
            0);
    }

    m_graphLine = vtkSmartPointer<vtkLineSource>::New();
    m_graphLine->SetPoints(graphPoints);
}

bool ImageProfileData::is3D() const
{
    return false;
}

RenderedData * ImageProfileData::createRendered()
{
    return new ImageProfilePlot(this);
}

QString ImageProfileData::dataTypeName() const
{
    return "image profile plot";
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

const QString & ImageProfileData::scalarsName() const
{
    return m_scalarsName;
}

const double * ImageProfileData::point1() const
{
    return m_point1;
}

const double * ImageProfileData::point2() const
{
    return m_point2;
}

QVtkTableModel * ImageProfileData::createTableModel()
{
    return nullptr;
}
