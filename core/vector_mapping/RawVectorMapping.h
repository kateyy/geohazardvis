#pragma once

#include <core/vector_mapping/VectorMappingData.h>


class vtkFloatArray;
class vtkEventQtSlotConnect;

class RawVectorData;
class PolyDataObject;


class RawVectorMapping : public VectorMappingData
{
    Q_OBJECT

public:
    ~RawVectorMapping() override;

    QString name() const override;

    vtkIdType maximumStartingIndex() override;

protected:
    static QList<VectorMappingData *> newInstances(RenderedData * renderedData);

    RawVectorMapping(RenderedData * renderedData, RawVectorData * rawVector);

    void initialize() override;

    void startingIndexChangedEvent() override;

protected slots:
    void updateForChangedData();

private:
    static const bool s_registered;

    RawVectorData * m_rawVector;
    PolyDataObject * m_polyData;

    vtkSmartPointer<vtkFloatArray> m_sectionArray;

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;
};
