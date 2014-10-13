#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkImageData;
class vtkAssignAttribute;


class VectorGrid3DDataObject : public DataObject
{
public:
    VectorGrid3DDataObject(QString name, vtkImageData * dataSet);
    ~VectorGrid3DDataObject() override;

    bool is3D() const override;

    /** create a rendered instance */
    RenderedData * createRendered() override;

    QString dataTypeName() const override;

    /** @return vtkImageData with 3-component vectors assigned to point scalars */
    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

protected:
    QVtkTableModel * createTableModel() override;

private:
    vtkSmartPointer<vtkAssignAttribute> m_vectorsToScalars;
};
