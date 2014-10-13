#include "VectorGrid3DDataObject.h"

#include <vtkImageData.h>
#include <vtkAssignAttribute.h>

#include <core/data_objects/RenderedVectorGrid3D.h>
#include <core/table_model/QVtkTableModelVectorGrid3D.h>


namespace
{
const QString s_dataTypeName = "3D vector grid";
}

VectorGrid3DDataObject::VectorGrid3DDataObject(QString name, vtkImageData * dataSet)
    : DataObject(name, dataSet)
    , m_vectorsToScalars(vtkSmartPointer<vtkAssignAttribute>::New())
{
    m_vectorsToScalars->SetInputData(dataSet);
    m_vectorsToScalars->Assign(vtkDataSetAttributes::VECTORS, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);
}

VectorGrid3DDataObject::~VectorGrid3DDataObject() = default;

bool VectorGrid3DDataObject::is3D() const
{
    return true;
}

RenderedData * VectorGrid3DDataObject::createRendered()
{
    return new RenderedVectorGrid3D(this);
}

QString VectorGrid3DDataObject::dataTypeName() const
{
    return s_dataTypeName;
}

vtkDataSet * VectorGrid3DDataObject::processedDataSet()
{
    m_vectorsToScalars->Update();
    return vtkDataSet::SafeDownCast(m_vectorsToScalars->GetOutput());
}

vtkAlgorithmOutput * VectorGrid3DDataObject::processedOutputPort()
{
    return m_vectorsToScalars->GetOutputPort();
}

QVtkTableModel * VectorGrid3DDataObject::createTableModel()
{
    QVtkTableModel * model = new QVtkTableModelVectorGrid3D();
    model->setDataObject(this);

    return model;
}
