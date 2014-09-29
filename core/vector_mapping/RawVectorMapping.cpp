#include "RawVectorMapping.h"

#include <vtkFloatArray.h>
#include <vtkDataSet.h>
#include <vtkPoints.h>

#include <vtkPointData.h>
#include <vtkCellData.h>

#include <vtkCellCenters.h>
#include <vtkAssignAttribute.h>
#include <vtkGlyph3D.h>

#include <vtkEventQtSlotConnect.h>

#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include <core/DataSetHandler.h>
#include <core/vtkhelper.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "attribute array vectors";
}

const bool RawVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<VectorsForSurfaceMapping *> RawVectorMapping::newInstances(RenderedData * renderedData)
{
    QList<RawVectorData *> attrs;

    for (RawVectorData * attr : DataSetHandler::instance().rawVectors())
    {
        if (attr->dataArray()->GetNumberOfTuples() >= renderedData->dataObject()->dataSet()->GetNumberOfCells())
            attrs << attr;
    }

    QList<VectorsForSurfaceMapping *> instances;
    for (RawVectorData * attr : attrs)
    {
        RawVectorMapping * mapping = new RawVectorMapping(renderedData, attr);
        if (mapping->isValid())
        {
            mapping->initialize();
            instances << mapping;
        }
        else
            delete mapping;
    }

    return instances;
}

RawVectorMapping::RawVectorMapping(RenderedData * renderedData, RawVectorData * rawVector)
    : VectorsForSurfaceMapping(renderedData)
    , m_rawVector(rawVector)
{
    if (!m_isValid)
        return;

    arrowGlyph()->SetVectorModeToUseVector();
}

RawVectorMapping::~RawVectorMapping() = default;

QString RawVectorMapping::name() const
{
    assert(m_rawVector);
    return QString::fromLatin1(m_rawVector->dataArray()->GetName());
}

vtkIdType RawVectorMapping::maximumStartingIndex()
{
    vtkIdType diff = m_rawVector->dataArray()->GetNumberOfTuples()
        - renderedData()->dataObject()->dataSet()->GetNumberOfCells();

    assert(diff >= 0);

    return diff;
}

void RawVectorMapping::initialize()
{
    vtkFloatArray * dataArray = m_rawVector->dataArray();
    QByteArray sectionName = (QString::fromLatin1(m_rawVector->dataArray()->GetName()) + "_" + QString::number((vtkIdType)this)).toLatin1();
    vtkIdType numComponents = dataArray->GetNumberOfComponents();
    vtkIdType numTuples = dataArray->GetNumberOfTuples() - startingIndex();

    m_sectionArray = vtkSmartPointer<vtkFloatArray>::New();
    m_sectionArray->GetInformation()->Set(DataObject::ArrayIsAuxiliaryKey(), true);
    m_sectionArray->SetName(sectionName.data());
    m_sectionArray->SetNumberOfComponents(numComponents);

    m_sectionArray->SetNumberOfTuples(numTuples);
    m_sectionArray->SetArray(dataArray->GetPointer(startingIndex() * numComponents), numTuples * numComponents, true);

    polyData()->GetCellData()->AddArray(m_sectionArray);

    VTK_CREATE(vtkCellCenters, centroidPoints);
    centroidPoints->SetInputData(polyData());

    VTK_CREATE(vtkAssignAttribute, assignAttribute);
    assignAttribute->SetInputConnection(centroidPoints->GetOutputPort());
    assignAttribute->Assign(m_sectionArray->GetName(), vtkDataSetAttributes::VECTORS, vtkAssignAttribute::POINT_DATA);

    arrowGlyph()->SetInputConnection(assignAttribute->GetOutputPort());

    m_vtkQtConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    m_vtkQtConnect->Connect(dataArray, vtkCommand::ModifiedEvent, this, SLOT(updateForChangedData()));
}

void RawVectorMapping::startingIndexChangedEvent()
{
    vtkFloatArray * dataArray = m_rawVector->dataArray();
    vtkIdType numComponents = dataArray->GetNumberOfComponents();
    vtkIdType numTuples = dataArray->GetNumberOfTuples() - startingIndex();

    m_sectionArray->SetNumberOfTuples(numTuples);
    m_sectionArray->SetArray(dataArray->GetPointer(startingIndex() * numComponents), numTuples * numComponents, true);

    m_sectionArray->Modified();

    emit geometryChanged();
}

void RawVectorMapping::updateForChangedData()
{
    m_sectionArray->Modified();

    emit geometryChanged();
}
