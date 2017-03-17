#pragma once

#include <memory>
#include <utility>

#include <vtkSmartPointer.h>
#include <vtkVector.h>

#include <QString>

#include <core/types.h>
#include <core/CoordinateSystems.h>


class vtkDataArray;
class DataObject;


class CORE_API DataSetResidualHelper
{
public:
    DataSetResidualHelper();
    virtual ~DataSetResidualHelper();

    void setObservationDataObject(DataObject * observationDataObject);
    DataObject * observationDataObject();
    void setObservationScalars(const QString & name, IndexType location);
    void setObservationScalarsScale(double scale);
    double observationScalarsScale() const;

    void setModelDataObject(DataObject * modelDataObject);
    DataObject * modelDataObject();
    void setModelScalars(const QString & name, IndexType location);
    void setModelScalarsScale(double scale);
    double modelScalarsScale() const;

    /**
     * Specify the name for new residual data objects.
     * This must be set before calling updateResidual().
     */
    void setResidualDataObjectName(const QString & name);
    const QString & residualDataObjectName() const;

    DataObject * residualDataObject();
    std::unique_ptr<DataObject> takeResidualDataObject();

    enum class InputData
    {
        Observation,
        Model
    };

    void setGeometrySource(InputData inputData);
    InputData geometrySource() const;

    void setTargetCoordinateSystem(const CoordinateSystemSpecification & spec);
    const CoordinateSystemSpecification & targetCoordinateSystem() const;

    void setDeformationLineOfSight(const vtkVector3d & lineOfSight);
    const vtkVector3d & deformationLineOfSight() const;

    void setProjectedAttributeNameSuffix(const QString & suffix);
    const QString & projectedAttributeNameSuffix() const;

    bool isSetupComplete() const;

    /**
     * Project deformation vectors to line of sight displacements, where applicable.
     * If projects are applied, the projected scalars will be added to the observation/model
     * point or cell attribute so that the projected scalars can be used in visualization.
     * @return whether observation and model displacements are valid.
     */
    bool projectDisplacementsToLineOfSight();
    /**
     * Compute the residual.
     * This will only have an effect if isSetupComplete() returns true.
     */
    bool updateResidual();

    const QString & losObservationScalarsName() const;
    const QString & losModelScalarsName() const;

private:
    void invalidateResults();
    void invalidateResidual();

private:
    struct ScalarDef
    {
        ScalarDef();
        bool isComplete() const;
        QString name;
        IndexType location;
        double scale;
        QString projectedName;
        vtkSmartPointer<vtkDataArray> losDisplacements;
    };

    DataObject * m_observationDataObject;
    ScalarDef m_observationScalars;
    DataObject * m_modelDataObject;
    ScalarDef m_modelScalars;

    QString m_residualDataObjectName;
    std::unique_ptr<DataObject> m_residualDataObject;

    InputData m_geometrySource;
    CoordinateSystemSpecification m_targetCoordinateSystem;
    vtkVector3d m_lineOfSight;
    QString m_projectedAttributeSuffix;
};
