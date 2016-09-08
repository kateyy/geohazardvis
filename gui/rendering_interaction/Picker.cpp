#include "Picker.h"

#include <cassert>
#include <vector>

#include <QDebug>

#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkCellPicker.h>
#include <vtkImageSlice.h>
#include <vtkImageMapper3D.h>
#include <vtkInformation.h>
#include <vtkInformationStringKey.h>
#include <vtkMapper.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPropCollection.h>
#include <vtkPropPicker.h>
#include <vtkVector.h>
#include <vtkVersionMacros.h>
#include <vtkWeakPointer.h>

#include <core/types.h>
#include <core/color_mapping/ColorMapping.h>
#include <core/color_mapping/ColorMappingData.h>
#include <core/data_objects/PolyDataObject.h>
#include <core/rendered_data/RenderedData.h>
#include <core/utility/DataExtent.h>
#include <core/utility/macros.h>


class Picker_private
{
public:
    explicit Picker_private(Picker & picker)
        : q_ptr{ picker }
        , propPicker{ vtkSmartPointer<vtkPropPicker>::New() }
        , cellPicker{ vtkSmartPointer<vtkCellPicker>::New() }
        , pointPicker{ vtkSmartPointer<vtkPointPicker>::New() }
    {
        cellPicker->PickFromListOn();
        pointPicker->PickFromListOn();
    }

    void appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData);
    void appendGenericPositionInfo(QTextStream & stream, vtkDataSet & dataSet);
    static void appendPositionInfo(QTextStream & stream, vtkIdType index, const vtkVector3d & position,
        const QString & indexPrefix, const QString & coordinatePrefix);

    void appendScalarInfo(QTextStream & stream, int activeComponent, bool scalarsAreMapped);


    vtkSmartPointer<vtkPropPicker> propPicker;
    vtkSmartPointer<vtkCellPicker> cellPicker;
    vtkSmartPointer<vtkPointPicker> pointPicker;

    QString pickedObjectInfoString;
    VisualizationSelection pickedObjectInfo;
    vtkWeakPointer<vtkDataArray> pickedScalarArray;

    void operator=(const Picker_private &) = delete;

private:
    Picker & q_ptr;
};


Picker::Picker()
    : d_ptr{ std::make_unique<Picker_private>(*this) }
{
}

Picker::~Picker() = default;

Picker::Picker(Picker && other)
    : d_ptr{ std::move(other.d_ptr) }
{
}

void Picker::pick(const vtkVector2i & clickPosXY, vtkRenderer & renderer)
{
    d_ptr->pickedObjectInfoString.clear();
    d_ptr->pickedObjectInfo.clear();
    d_ptr->pickedScalarArray = nullptr;

    // pick points first; if this picker does not hit, we will also not hit cells
    d_ptr->propPicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

    auto prop3D = d_ptr->propPicker->GetProp3D();
    if (!prop3D)
    {
        return;
    }

    vtkAbstractMapper * abstractMapper = nullptr;
    vtkImageSlice * imageSlice = nullptr;

    if (auto actor = d_ptr->propPicker->GetActor())
    {
        abstractMapper = actor->GetMapper();
    }
    else
    {
        imageSlice = vtkImageSlice::SafeDownCast(prop3D);
        assert(imageSlice);
        if (imageSlice)
        {
            abstractMapper = imageSlice->GetMapper();
        }
    }

    if (!abstractMapper)
    {
        qDebug() << "Unknown object picked at: " + QString::number(clickPosXY[0]) + " " + QString::number(clickPosXY[1]);
        return;
    }

    auto & mapperInfo = *abstractMapper->GetInformation();

    d_ptr->pickedObjectInfo.visualization = AbstractVisualizedData::readPointer(mapperInfo);
    if (!d_ptr->pickedObjectInfo.visualization)
    {
        qDebug() << "no visualization referenced in mapper";
        return;
    }

    auto & visualization = *d_ptr->pickedObjectInfo.visualization;

    // extract data values encoded by the color mapping
    auto & colorMapping = visualization.colorMapping();
    auto & colorMappingData = colorMapping.currentScalars();
    if (colorMapping.isEnabled())
    {
        d_ptr->pickedObjectInfo.indexType = colorMappingData.scalarsAssociation(visualization);
    }

    // TODO determine correct visualization port
    d_ptr->pickedObjectInfo.visOutputPort = visualization.defaultVisualizationPort();

    QString content;
    QTextStream stream;
    stream.setString(&content);

    stream.setRealNumberNotation(QTextStream::RealNumberNotation::ScientificNotation);
    stream.setRealNumberPrecision(5);

    auto inputName = DataObject::readName(mapperInfo);
    if (inputName.isEmpty())
    {
        inputName = "(unnamed)";
    }
    else
    {
        inputName = "\"" + inputName + "\"";
    }

    stream << "Data Set: " << inputName << endl;


    const auto polyData = dynamic_cast<PolyDataObject *>(&visualization.dataObject());
    const bool isPolyDataObject = polyData != nullptr;

    // no active color mapping -> pick default locations
    if (d_ptr->pickedObjectInfo.indexType == IndexType::invalid)
    {
        if (isPolyDataObject)
        {
            d_ptr->pickedObjectInfo.indexType = IndexType::cells;
        }
        else
        {
            d_ptr->pickedObjectInfo.indexType = IndexType::points;
        }
    }

    assert(d_ptr->pickedObjectInfo.indexType != IndexType::invalid);

    assert(dynamic_cast<RenderedData *>(d_ptr->pickedObjectInfo.visualization));
    auto renderedData = static_cast<RenderedData *>(d_ptr->pickedObjectInfo.visualization);

    auto activePicker = d_ptr->pickedObjectInfo.indexType == IndexType::points
        ? static_cast<vtkPicker *>(d_ptr->pointPicker)
        : static_cast<vtkPicker *>(d_ptr->cellPicker);
    assert(activePicker);

    auto pickList = activePicker->GetPickList();
    pickList->RemoveAllItems();
    auto viewProps = renderedData->viewProps();
    vtkCollectionSimpleIterator viewPropIt;
    for (viewProps->InitTraversal(viewPropIt); auto prop = viewProps->GetNextProp(viewPropIt);)
    {
        pickList->AddItem(prop);
    }
    
    activePicker->Pick(clickPosXY[0], clickPosXY[1], 0, &renderer);

    if (!activePicker->GetDataSet())
    {
        d_ptr->pickedObjectInfo.indexType = IndexType::invalid;
        d_ptr->pickedObjectInfoString = stream.readAll();
        return;
    }

    auto & dataSet = *activePicker->GetDataSet();

    if (d_ptr->pickedObjectInfo.indexType == IndexType::cells)
    {
        d_ptr->pickedObjectInfo.setIndex(d_ptr->cellPicker->GetCellId());
    }
    else
    {
        d_ptr->pickedObjectInfo.setIndex(d_ptr->pointPicker->GetPointId());
    }

    if (d_ptr->pickedObjectInfo.isIndexListEmpty())
    {
        d_ptr->pickedObjectInfo.indexType = IndexType::invalid;
        d_ptr->pickedObjectInfoString = stream.readAll();
        return;
    }

    // ----------------------------
    // object type specific picking
    // ----------------------------

    if (isPolyDataObject)
    {
        // poly data: may have point or cell scalars
        d_ptr->appendPolyDataInfo(stream, *polyData);
    }
    else // point based data sets: images, volumes, volume slices, glyphs
    {
        d_ptr->appendGenericPositionInfo(stream, dataSet);
    }

    if (!d_ptr->pickedObjectInfo.isIndexListEmpty() && colorMapping.isEnabled())
    {
        auto attributes = d_ptr->pickedObjectInfo.indexType == IndexType::cells
            ? static_cast<vtkDataSetAttributes *>(dataSet.GetCellData())
            : static_cast<vtkDataSetAttributes *>(dataSet.GetPointData());

        d_ptr->pickedScalarArray = attributes->GetArray(colorMappingData.scalarsName(visualization).toUtf8().data());

        if (d_ptr->pickedScalarArray)
        {
            d_ptr->appendScalarInfo(stream, colorMappingData.dataComponent(), colorMappingData.mapsScalarsToColors());
        }
    }

    if (!isPolyDataObject && !imageSlice)
    {
        // TODO check how to match (resampled) glyph/volume slice indices with input data indices
        d_ptr->pickedObjectInfo.indexType = IndexType::invalid;
        d_ptr->pickedObjectInfo.indices.clear();
    }

    d_ptr->pickedObjectInfoString = stream.readAll();
}

const QString & Picker::pickedObjectInfoString() const
{
    return d_ptr->pickedObjectInfoString;
}

const VisualizationSelection & Picker::pickedObjectInfo() const
{
    return d_ptr->pickedObjectInfo;
}

vtkDataArray * Picker::pickedScalarArray()
{
    return d_ptr->pickedScalarArray;
}

void Picker_private::appendPolyDataInfo(QTextStream & stream, PolyDataObject & polyData)
{
    const auto pickedIndex = pickedObjectInfo.indices.front();
    vtkVector3d position;
    QString indexPrefix, coordinatePrefix;

    if (pickedObjectInfo.indexType == IndexType::cells)
    {
        polyData.cellCenters()->GetPoint(pickedIndex, position.GetData());
        indexPrefix = "Triangle";
        coordinatePrefix = "Centroid";
    }
    else
    {
        polyData.processedDataSet()->GetPoint(pickedIndex, position.GetData());
        stream << endl
            << "Point Index: " << pickedIndex << endl
            << "X = " << position[0] << endl
            << "Y = " << position[1] << endl
            << "Z = " << position[2];
        indexPrefix = "Point";
    }

    appendPositionInfo(stream, pickedIndex, position, indexPrefix, coordinatePrefix);
}

void Picker_private::appendGenericPositionInfo(QTextStream & stream, vtkDataSet & dataSet)
{
    const auto pickedIndex = pickedObjectInfo.indices.front();
    vtkVector3d position;
    QString indexPrefix, coordinatePrefix;

    if (pickedObjectInfo.indexType == IndexType::points)
    {
        dataSet.GetPoint(pickedIndex, position.GetData());
        indexPrefix = "Point";
    }
    else
    {
        DataBounds cellBounds;
        dataSet.GetCellBounds(pickedIndex, cellBounds.data());
        position = cellBounds.center();
        indexPrefix = "Grid";
        coordinatePrefix = "Centroid";
    }

    appendPositionInfo(stream, pickedIndex, position, indexPrefix, coordinatePrefix);
}

void Picker_private::appendPositionInfo(QTextStream & stream, vtkIdType index, const vtkVector3d & position,
    const QString & indexPrefix, const QString & coordinatePrefix)
{
    stream
        << indexPrefix << (indexPrefix.isEmpty() ? "" : " ") << "Index: " << index << endl;
    QString coordinateIndent;
    if (!coordinatePrefix.isEmpty())
    {
        stream
            << coordinatePrefix << ":" << endl;
        coordinateIndent = "    ";
    }
    stream
        << coordinateIndent << "X = " << position[0] << endl
        << coordinateIndent << "Y = " << position[1] << endl
        << coordinateIndent << "Z = " << position[2];
}

void Picker_private::appendScalarInfo(QTextStream & stream, int activeComponent, bool scalarsAreMapped)
{
    assert(pickedScalarArray && !pickedObjectInfo.isIndexListEmpty());
    auto & scalars = *pickedScalarArray;
    const auto index = pickedObjectInfo.indices.front();
    auto tuple = std::vector<double>(scalars.GetNumberOfComponents());
    scalars.GetTuple(index, tuple.data());

    QString unitStr;
#if VTK_CHECK_VERSION(7, 1, 0)
    unitStr = scalars.GetInformation()->Get(vtkDataArray::UNITS_LABEL());
    if (!unitStr.isEmpty())
    {
        unitStr.prepend(" ");
    }
#endif

    const QString locationString = pickedObjectInfo.indexType == IndexType::cells
        ? "Cell"
        : "Point";

    if (scalarsAreMapped)
    {
        assert(activeComponent >= 0);

        stream << endl << endl << locationString << " Attribute: \"" << QString::fromUtf8(scalars.GetName()) << "\"";
        if (tuple.size() > 1)
        {
            stream << " (active component: " << activeComponent + 1 << ")";
            for (size_t i = 0; i < tuple.size(); ++i)
            {
                stream << endl << "[" << i + 1 << "] " << tuple[i] << unitStr;
                if (i == activeComponent)
                {
                    stream << " *";
                }
            }
        }
        else
        {
            stream << endl << "Value: " << tuple[activeComponent] << unitStr;
        }
    }
    else if (tuple.size() == 3 || tuple.size() == 4)
    {
        stream << endl << locationString << " Color" << endl;
        stream << "R: " << tuple[0] << unitStr << endl;
        stream << "G: " << tuple[1] << unitStr << endl;
        stream << "B: " << tuple[2] << unitStr;
        if (tuple.size() == 4)
        {
            stream << endl << "A: " << tuple[3] << unitStr;
        }
    }
}
