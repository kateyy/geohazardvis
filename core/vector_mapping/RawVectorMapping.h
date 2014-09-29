#pragma once

#include <core/vector_mapping/VectorsForSurfaceMapping.h>


class vtkFloatArray;
class vtkEventQtSlotConnect;

class RawVectorData;


class RawVectorMapping : public VectorsForSurfaceMapping
{
    Q_OBJECT

public:
    ~RawVectorMapping() override;

    QString name() const override;

    vtkIdType maximumStartingIndex() override;

protected:
    static QList<VectorsForSurfaceMapping *> newInstances(RenderedData * renderedData);

    RawVectorMapping(RenderedData * renderedData, RawVectorData * rawVector);

    void initialize() override;

    void startingIndexChangedEvent() override;

protected slots:
    void updateForChangedData();

private:
    static const bool s_registered;

    RawVectorData * m_rawVector;

    vtkSmartPointer<vtkFloatArray> m_sectionArray;

    vtkSmartPointer<vtkEventQtSlotConnect> m_vtkQtConnect;
};
