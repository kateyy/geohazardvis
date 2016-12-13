#pragma once

#include <vtkSmartPointer.h>

#include <core/data_objects/GenericPolyDataObject.h>


class vtkPolyDataNormals;
class vtkCellCenters;


class CORE_API PolyDataObject : public GenericPolyDataObject
{
public:
    PolyDataObject(const QString & name, vtkPolyData & dataSet);
    ~PolyDataObject() override;

    IndexType defaultAttributeLocation() const override;

    void addDataArray(vtkDataArray & dataArray) override;

    /** @return centroids with normals, computed from polygonal data set cells */
    vtkPolyData * cellCenters();
    vtkAlgorithmOutput * cellCentersOutputPort();

    std::unique_ptr<RenderedData> createRendered() override;

    const QString & dataTypeName() const override;
    static const QString & dataTypeName_s();

    bool is2p5D();

    double cellCenterComponent(vtkIdType cellId, int component, bool * validIdPtr = nullptr);
    bool setCellCenterComponent(vtkIdType cellId, int component, double value);
    double cellNormalComponent(vtkIdType cellId, int component, bool * validIdPtr = nullptr);
    bool setCellNormalComponent(vtkIdType cellId, int component, double value);

protected:
    /** @return poly data set with cell normals */
    vtkAlgorithmOutput * processedOutputPortInternal() override;

    std::unique_ptr<QVtkTableModel> createTableModel() override;

private:
    void setupCellCenters();

private:
    vtkSmartPointer<vtkPolyDataNormals> m_cellNormals;
    vtkSmartPointer<vtkCellCenters> m_cellCenters;

    enum class Is2p5D { yes, no, unchecked };
    Is2p5D m_is2p5D;

private:
    Q_DISABLE_COPY(PolyDataObject)
};
