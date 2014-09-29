#include "AttributeVectorMapping.h"

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
#include <core/data_objects/AttributeVectorData.h>
#include <core/data_objects/RenderedData.h>
#include <core/vector_mapping/VectorsForSurfaceMappingRegistry.h>


namespace
{
const QString s_name = "attribute array vectors";
}

const bool AttributeVectorMapping::s_registered = VectorsForSurfaceMappingRegistry::instance().registerImplementation(
    s_name,
    newInstances);

QList<VectorsForSurfaceMapping *> AttributeVectorMapping::newInstances(RenderedData * renderedData)
{
    QList<AttributeVectorData *> attrs;

    for (AttributeVectorData * attr : DataSetHandler::instance().attributeVectors())
    {
        if (attr->dataArray()->GetNumberOfTuples() >= renderedData->dataObject()->dataSet()->GetNumberOfCells())
            attrs << attr;
    }

    QList<VectorsForSurfaceMapping *> instances;
    for (AttributeVectorData * attr : attrs)
    {
        AttributeVectorMapping * mapping = new AttributeVectorMapping(renderedData, attr);
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

AttributeVectorMapping::AttributeVectorMapping(RenderedData * renderedData, AttributeVectorData * attributeVector)
    : VectorsForSurfaceMapping(renderedData)
    , m_attributeVector(attributeVector)
{
    if (!m_isValid)
        return;

    arrowGlyph()->SetVectorModeToUseVector();
}

AttributeVectorMapping::~AttributeVectorMapping() = default;

QString AttributeVectorMapping::name() const
{
    assert(m_attributeVector);
    return QString::fromLatin1(m_attributeVector->dataArray()->GetName());
}

vtkIdType AttributeVectorMapping::maximumStartingIndex()
{
    vtkIdType diff = m_attributeVector->dataArray()->GetNumberOfTuples()
        - renderedData()->dataObject()->dataSet()->GetNumberOfCells();

    assert(diff >= 0);

    return diff;
}

void AttributeVectorMapping::initialize()
{
    vtkFloatArray * dataArray = m_attributeVector->dataArray();
    QByteArray sectionName = (QString::fromLatin1(m_attributeVector->dataArray()->GetName()) + "_" + QString::number((vtkIdType)this)).toLatin1();
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

void AttributeVectorMapping::startingIndexChangedEvent()
{
    vtkFloatArray * dataArray = m_attributeVector->dataArray();
    vtkIdType numComponents = dataArray->GetNumberOfComponents();
    vtkIdType numTuples = dataArray->GetNumberOfTuples() - startingIndex();

    m_sectionArray->SetNumberOfTuples(numTuples);
    m_sectionArray->SetArray(dataArray->GetPointer(startingIndex() * numComponents), numTuples * numComponents, true);

    m_sectionArray->Modified();

    emit geometryChanged();
}

void AttributeVectorMapping::updateForChangedData()
{
    m_sectionArray->Modified();

    emit geometryChanged();
}
