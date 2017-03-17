#include "DataSetResidualHelper.h"

#include <type_traits>

#include <QDebug>

#include <vtkAOSDataArrayTemplate.h>
#include <vtkArrayDispatch.h>
#include <vtkAssume.h>
#include <vtkCellData.h>
#include <vtkDataArrayAccessor.h>
#include <vtkDataSet.h>
#include <vtkPointData.h>
#include <vtkSMPTools.h>

#include <core/data_objects/CoordinateTransformableDataObject.h>
#include <core/utility/InterpolationHelper.h>
#include <core/utility/vtkvectorhelper.h>


namespace
{

struct ProjectToLineOfSightWorker
{
    vtkVector3d lineOfSight;
    vtkSmartPointer<vtkDataArray> projectedData;

    template<typename Vector_t>
    void operator()(Vector_t * vectors)
    {
        VTK_ASSUME(vectors->GetNumberOfComponents() == 3);

        using ValueType = typename vtkDataArrayAccessor<Vector_t>::APIType;

        auto output = vtkSmartPointer<vtkAOSDataArrayTemplate<ValueType>>::New();
        output->SetNumberOfTuples(vectors->GetNumberOfTuples());

        lineOfSight.Normalize();
        const auto los = convertTo<ValueType>(lineOfSight);

        vtkDataArrayAccessor<Vector_t> v(vectors);
        vtkDataArrayAccessor<Vector_t> l(output);

        vtkSMPTools::For(0, vectors->GetNumberOfTuples(),
            [v, l, los] (vtkIdType begin, vtkIdType end)
        {
            vtkVector3<ValueType> vector;
            for (vtkIdType i = begin; i < end; ++i)
            {
                v.Get(i, vector.GetData());
                const auto projection = vector.Dot(los);
                l.Set(i, 0, projection);
            };
        });

        projectedData = output;
    }
};

struct ResidualWorker
{
    double observationUnitFactor;
    double modelUnitFactor;

    vtkSmartPointer<vtkDataArray> residual;

    template<typename Observation_t, typename Model_t>
    void operator()(Observation_t * observation, Model_t * model)
    {
        VTK_ASSUME(observation->GetNumberOfComponents() == 1);
        VTK_ASSUME(model->GetNumberOfComponents() == 1);
        VTK_ASSUME(model->GetNumberOfTuples() == observation->GetNumberOfTuples());

        using ObservationValue_t = typename vtkDataArrayAccessor<Observation_t>::APIType;
        using ModelValue_t = typename vtkDataArrayAccessor<Model_t>::APIType;
        using ResidualValue_t = std::common_type_t<ObservationValue_t, ModelValue_t>;

        auto res = vtkSmartPointer<vtkAOSDataArrayTemplate<ResidualValue_t>>::New();
        res->SetNumberOfValues(observation->GetNumberOfTuples());

        vtkDataArrayAccessor<Observation_t> o(observation);
        vtkDataArrayAccessor<Model_t> m(model);
        vtkDataArrayAccessor<vtkAOSDataArrayTemplate<ResidualValue_t>> r(res);

        const auto oUnit = static_cast<ObservationValue_t>(observationUnitFactor);
        const auto mUnit = static_cast<ModelValue_t>(modelUnitFactor);

        vtkSMPTools::For(0, res->GetNumberOfValues(),
            [o, m, r, oUnit, mUnit] (vtkIdType begin, vtkIdType end)
        {
            for (vtkIdType i = begin; i < end; ++i)
            {
                const auto diff = static_cast<ResidualValue_t>(
                    o.Get(i, 0) * oUnit
                    - m.Get(i, 0) * mUnit);
                r.Set(i, 0, diff);
            }
        });

        residual = res;
    }
};

}


DataSetResidualHelper::ScalarDef::ScalarDef()
    : name{}
    , location{ IndexType::invalid }
    , scale{ 1.0 }
    , projectedName{}
    , losDisplacements{}
{
}

bool DataSetResidualHelper::ScalarDef::isComplete() const
{
    return !name.isEmpty() && location != IndexType::invalid;
}


DataSetResidualHelper::DataSetResidualHelper()
    : m_observationDataObject{}
    , m_observationScalars{}
    , m_modelDataObject{}
    , m_modelScalars{}
    , m_residualDataObjectName{ "Residual" }
    , m_residualDataObject{}
    , m_geometrySource{ InputData::Observation }
    , m_targetCoordinateSystem{}
    , m_lineOfSight{ 0.0, 0.0, 1.0 }
    , m_projectedAttributeSuffix{ " (projected)" }
{
}

DataSetResidualHelper::~DataSetResidualHelper()
{
}

void DataSetResidualHelper::setObservationDataObject(DataObject * observationDataObject)
{
    if (m_observationDataObject == observationDataObject)
    {
        return;
    }

    invalidateResults();
    m_observationDataObject = observationDataObject;
}

DataObject * DataSetResidualHelper::observationDataObject()
{
    return m_observationDataObject;
}

void DataSetResidualHelper::setObservationScalars(const QString & name, IndexType location)
{
    if (m_observationScalars.name == name
        && m_observationScalars.location == location)
    {
        return;
    }

    invalidateResults();
    m_observationScalars.name = name;
    m_observationScalars.location = location;
}

void DataSetResidualHelper::setObservationScalarsScale(double scale)
{
    if (m_observationScalars.scale == scale)
    {
        return;
    }

    invalidateResidual();
    m_observationScalars.scale = scale;
}

double DataSetResidualHelper::observationScalarsScale() const
{
    return m_observationScalars.scale;
}

void DataSetResidualHelper::setModelDataObject(DataObject * modelDataObject)
{
    if (m_modelDataObject == modelDataObject)
    {
        return;
    }

    invalidateResults();
    m_modelDataObject = modelDataObject;
}

DataObject * DataSetResidualHelper::modelDataObject()
{
    return m_modelDataObject;
}

void DataSetResidualHelper::setModelScalars(const QString & name, IndexType location)
{
    if (m_modelScalars.name == name
        && m_modelScalars.location == location)
    {
        return;
    }

    invalidateResults();
    m_modelScalars.name = name;
    m_modelScalars.location = location;
}

void DataSetResidualHelper::setModelScalarsScale(double scale)
{
    if (m_modelScalars.scale == scale)
    {
        return;
    }

    invalidateResidual();
    m_modelScalars.scale = scale;
}

double DataSetResidualHelper::modelScalarsScale() const
{
    return m_modelScalars.scale;
}

void DataSetResidualHelper::setResidualDataObjectName(const QString & name)
{
    if (m_residualDataObjectName == name)
    {
        return;
    }

    invalidateResidual();
    m_residualDataObjectName = name;
}

const QString & DataSetResidualHelper::residualDataObjectName() const
{
    return m_residualDataObjectName;
}

DataObject * DataSetResidualHelper::residualDataObject()
{
    return m_residualDataObject.get();
}

std::unique_ptr<DataObject> DataSetResidualHelper::takeResidualDataObject()
{
    return std::move(m_residualDataObject);
}

void DataSetResidualHelper::setGeometrySource(InputData inputData)
{
    if (m_geometrySource == inputData)
    {
        return;
    }

    invalidateResidual();
    m_geometrySource = inputData;
}

auto DataSetResidualHelper::geometrySource() const -> InputData
{
    return m_geometrySource;
}

void DataSetResidualHelper::setTargetCoordinateSystem(const CoordinateSystemSpecification & spec)
{
    if (m_targetCoordinateSystem == spec)
    {
        return;
    }

    invalidateResidual();

    m_targetCoordinateSystem = spec;
}

const CoordinateSystemSpecification & DataSetResidualHelper::targetCoordinateSystem() const
{
    return m_targetCoordinateSystem;
}

void DataSetResidualHelper::setDeformationLineOfSight(const vtkVector3d & lineOfSight)
{
    if (m_lineOfSight == lineOfSight)
    {
        return;
    }

    invalidateResults();
    m_lineOfSight = lineOfSight;
}

const vtkVector3d & DataSetResidualHelper::deformationLineOfSight() const
{
    return m_lineOfSight;
}

void DataSetResidualHelper::setProjectedAttributeNameSuffix(const QString & suffix)
{
    if (m_projectedAttributeSuffix == suffix)
    {
        return;
    }

    invalidateResidual();
    m_projectedAttributeSuffix = suffix;
}

const QString & DataSetResidualHelper::projectedAttributeNameSuffix() const
{
    return m_projectedAttributeSuffix;
}

bool DataSetResidualHelper::isSetupComplete() const
{
    return m_observationDataObject && m_observationScalars.isComplete()
        && m_modelDataObject && m_modelScalars.isComplete();
}

bool DataSetResidualHelper::projectDisplacementsToLineOfSight()
{
    auto observationDS = m_observationDataObject ? m_observationDataObject->dataSet() : nullptr;
    auto modelDS = m_modelDataObject ? m_modelDataObject->dataSet() : nullptr;

    const vtkSmartPointer<vtkDataArray> observationData = observationDS
        ? (m_observationScalars.location == IndexType::cells
            ? observationDS->GetCellData()->GetArray(m_observationScalars.name.toUtf8().data())
            : observationDS->GetPointData()->GetArray(m_observationScalars.name.toUtf8().data()))
        : nullptr;

    const vtkSmartPointer<vtkDataArray> modelData = modelDS
        ? (m_modelScalars.location == IndexType::cells
            ? modelDS->GetCellData()->GetArray(m_modelScalars.name.toUtf8().data())
            : modelDS->GetPointData()->GetArray(m_modelScalars.name.toUtf8().data()))
        : nullptr;

    if (observationDS && !observationData)
    {
        qDebug() << "Could not find valid observation data for residual computation (" << m_observationScalars.name + ")";
    }

    if (modelDS && !modelData)
    {
        qDebug() << "Could not find valid model data for residual computation (" << m_modelScalars.name + ")";
    }

    if (!observationData && !modelData)
    {
        return false;
    }


    // project displacement vectors to the line of sight vector, if required
    // This also adds the projection result as scalars to the respective data set, so that it can be used
    // in the visualization.
    auto projectOrFetchDisplacement = [this] (vtkDataArray & displacement, vtkDataSet & dataSet,
        ScalarDef & scalarDef) -> bool {

        if (displacement.GetNumberOfComponents() == 1)
        {
            scalarDef.projectedName = "";
            scalarDef.losDisplacements = &displacement;  // is already projected
            return true;
        }

        if (displacement.GetNumberOfComponents() != 3)
        {   // can't handle that
            scalarDef.losDisplacements = {};
            return false;
        }

        using LosDispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals>;
        ProjectToLineOfSightWorker losWorker;
        losWorker.lineOfSight = m_lineOfSight;
        if (!LosDispatcher::Execute(&displacement, losWorker))
        {
            losWorker(&displacement);
        }

        auto projected = losWorker.projectedData;

        scalarDef.projectedName = scalarDef.name + m_projectedAttributeSuffix;
        projected->SetName(scalarDef.projectedName.toUtf8().data());

        if (scalarDef.location == IndexType::cells)
        {
            dataSet.GetCellData()->AddArray(projected);
        }
        else
        {
            dataSet.GetPointData()->AddArray(projected);
        }

        scalarDef.losDisplacements = projected;
        return true;
    };


    bool observationValid = false;
    bool modelValid = false;

    if (observationDS && observationData)
    {
        observationValid =
            projectOrFetchDisplacement(*observationData, *observationDS, m_observationScalars);
    }
    if (modelDS && modelData)
    {
        modelValid = projectOrFetchDisplacement(*modelData, *modelDS, m_modelScalars);
    }

    return observationValid && modelValid;
}

bool DataSetResidualHelper::updateResidual()
{
    if (!m_observationScalars.losDisplacements || !m_modelScalars.losDisplacements)
    {
        if (!projectDisplacementsToLineOfSight())
        {
            return false;
        }
    }

    if (!isSetupComplete())
    {
        m_observationScalars.projectedName.clear();
        m_modelScalars.projectedName.clear();

        return false;
    }

    auto observationLosDisp = m_observationScalars.losDisplacements;
    auto modelLosDisp = m_modelScalars.losDisplacements;
    assert(observationLosDisp && modelLosDisp);


    auto transformableObservation = dynamic_cast<CoordinateTransformableDataObject *>(m_observationDataObject);
    auto transformableModel = dynamic_cast<CoordinateTransformableDataObject *>(m_modelDataObject);
    const bool useTransformedCoordinates =
        transformableObservation && transformableModel && m_targetCoordinateSystem.isValid();

    auto observationDSTransformedPtr = useTransformedCoordinates
        ? transformableObservation->coordinateTransformedDataSet(m_targetCoordinateSystem)
        : vtkSmartPointer<vtkDataSet>(m_observationDataObject->dataSet());
    auto modelDSTransformedPtr = useTransformedCoordinates
        ? transformableModel->coordinateTransformedDataSet(m_targetCoordinateSystem)
        : vtkSmartPointer<vtkDataSet>(m_modelDataObject->dataSet());

    if (!observationDSTransformedPtr || !modelDSTransformedPtr)
    {
        qDebug() << "Invalid input coordinates or data sets";
        return false;
    }

    auto & observationDSTransformed = *observationDSTransformedPtr;
    auto & modelDSTransformed = *modelDSTransformedPtr;

    // Now interpolate one of the data arrays to the other's structure.
    // Use the data sets transformed to the user-selected coordinate system.
    if (m_geometrySource == InputData::Observation)
    {
        auto attributeName = QString::fromUtf8(modelLosDisp->GetName());
        modelLosDisp = InterpolationHelper::interpolate(
            observationDSTransformed, modelDSTransformed, attributeName,
            m_modelScalars.location == IndexType::cells,
            m_observationScalars.location == IndexType::cells);
    }
    else
    {
        auto attributeName = QString::fromUtf8(observationLosDisp->GetName());
        observationLosDisp = InterpolationHelper::interpolate(
            modelDSTransformed, observationDSTransformed, attributeName,
            m_observationScalars.location == IndexType::cells,
            m_modelScalars.location == IndexType::cells);
    }

    if (!observationLosDisp || !modelLosDisp)
    {
        qDebug() << "Observation/Model interpolation failed";

        return false;
    }


    // expect line of sight displacements, matching to one of the structures here
    assert(modelLosDisp->GetNumberOfComponents() == 1);
    assert(observationLosDisp->GetNumberOfComponents() == 1);
    assert(modelLosDisp->GetNumberOfTuples() == observationLosDisp->GetNumberOfTuples());


    // compute the residual data

    auto & referenceDataSet = m_geometrySource == InputData::Observation
        ? observationDSTransformed
        : modelDSTransformed;

    using ResidualDispatcher = vtkArrayDispatch::Dispatch2ByValueType<
        vtkArrayDispatch::Reals, vtkArrayDispatch::Reals>;
    ResidualWorker residualWorker;
    residualWorker.observationUnitFactor = m_observationScalars.scale;
    residualWorker.modelUnitFactor = m_modelScalars.scale;
    if (!ResidualDispatcher::Execute(observationLosDisp, modelLosDisp, residualWorker))
    {
        residualWorker(observationLosDisp.Get(), modelLosDisp.Get());
    }
    auto residualData = residualWorker.residual;
    residualData->SetName(m_residualDataObjectName.toUtf8().data());

    auto newResidual = vtkSmartPointer<vtkDataSet>::Take(referenceDataSet.NewInstance());
    newResidual->CopyStructure(&referenceDataSet);

    const auto resultAttributeLocation = m_geometrySource == InputData::Observation
        ? m_observationScalars.location : m_modelScalars.location;
    const auto residualExpectedTuples = resultAttributeLocation == IndexType::cells
        ? newResidual->GetNumberOfCells()
        : newResidual->GetNumberOfPoints();

    if (residualData->GetNumberOfTuples() != residualExpectedTuples)
    {
        qDebug() << "Residual creation failed: Unexpected output size.";
        return false;
    }

    auto & resultAttributes = resultAttributeLocation == IndexType::cells
        ? static_cast<vtkDataSetAttributes &>(*newResidual->GetCellData())
        : static_cast<vtkDataSetAttributes &>(*newResidual->GetPointData());
    resultAttributes.SetScalars(residualData);

    const auto & sourceDataObject = m_geometrySource == InputData::Observation
        ? *m_observationDataObject : *m_modelDataObject;

    m_residualDataObject = sourceDataObject.newInstance(m_residualDataObjectName, newResidual);

    if (auto transformableSource = m_geometrySource == InputData::Observation
        ? transformableObservation : transformableModel)
    {
        auto trResidual = static_cast<CoordinateTransformableDataObject *>(m_residualDataObject.get());
        trResidual->specifyCoordinateSystem(transformableSource->coordinateSystem());
    }

    return m_residualDataObject != nullptr;
}

const QString & DataSetResidualHelper::losObservationScalarsName() const
{
    if (!m_observationScalars.projectedName.isEmpty())
    {
        return m_observationScalars.projectedName;
    }

    return m_observationScalars.name;
}

const QString & DataSetResidualHelper::losModelScalarsName() const
{
    if (!m_modelScalars.projectedName.isEmpty())
    {
        return m_modelScalars.projectedName;
    }

    return m_modelScalars.name;
}

void DataSetResidualHelper::invalidateResults()
{
    invalidateResidual();
    m_observationScalars.projectedName.clear();
    m_observationScalars.losDisplacements = {};
    m_modelScalars.projectedName.clear();
    m_modelScalars.losDisplacements = {};
}

void DataSetResidualHelper::invalidateResidual()
{
    m_residualDataObject = {};
}
