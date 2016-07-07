#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/DataObject.h>


class vtkPolyData;
class vtkPolyDataNormals;
class vtkCellCenters;
class vtkAlgorithmOutput;


class CORE_API PolyDataObject : public DataObject
{
public:
    PolyDataObject(const QString & name, vtkPolyData & dataSet);

    bool is3D() const override;

    vtkPolyData * polyDataSet();
    const vtkPolyData * polyDataSet() const;

    /** @return poly data set with cell normals */
    vtkDataSet * processedDataSet() override;
    vtkAlgorithmOutput * processedOutputPort() override;

    void addDataArray(vtkDataArray & dataArray) override;

    /** @return centroids with normals, computed from polygonal data set cells */
    vtkPolyData * cellCenters();
    vtkAlgorithmOutput * cellCentersOutputPort();

    std::unique_ptr<RenderedData> createRendered() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    bool is2p5D();

    bool setCellCenterComponent(vtkIdType cellId, int component, double value);
    bool setCellNormalComponent(vtkIdType cellId, int component, double value);

protected:
    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    void setupCellCenters();

private:
    vtkSmartPointer<vtkPolyDataNormals> m_cellNormals;
    vtkSmartPointer<vtkCellCenters> m_cellCenters;

    enum class Is2p5D { yes, no, unchecked };
    Is2p5D m_is2p5D;
};
