#pragma once

#include <QMap>

#include <vtkSmartPointer.h>

#include <core/scalar_mapping/AbstractArrayComponentMapping.h>


class vtkFloatArray;


class CORE_API RawArrayComponentMapping : public AbstractArrayComponentMapping
{
public:
    RawArrayComponentMapping(const QList<DataObject *> & dataObjects, vtkFloatArray * dataArray, vtkIdType component);
    ~RawArrayComponentMapping() override;

    vtkIdType maximumStartingIndex() override;

    vtkAlgorithm * createFilter(DataObject * dataObject) override;
    bool usesFilter() const override;

    void configureDataObjectAndMapper(DataObject * dataObject, vtkMapper * mapper) override;

protected:
    static QList<ScalarsForColorMapping *> newInstances(const QList<DataObject *> & dataObjects);

    void updateBounds() override;

    void startingIndexChangedEvent() override;
    void objectOrderChangedEvent() override;

private:
    QByteArray arraySectionName(DataObject * dataObject);
    vtkIdType sectionIndex(DataObject * dataObject);

    void redistributeArraySections();

private:
    static const bool s_registered;

    QMap<DataObject *, vtkIdType> m_dataObjectToArrayIndex;

    vtkFloatArray * m_dataArray;
    QMap<DataObject *, vtkSmartPointer<vtkFloatArray>> m_sections;
};
