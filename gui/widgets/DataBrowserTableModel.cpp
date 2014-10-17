#include "DataBrowserTableModel.h"

#include <cassert>
#include <algorithm>

#include <vtkDataSet.h>
#include <vtkFloatArray.h>

#include <core/DataSetHandler.h>
#include <core/data_objects/RawVectorData.h>
#include <core/data_objects/ImageDataObject.h>
#include <core/data_objects/ImageProfileData.h>
#include <core/data_objects/VectorGrid3DDataObject.h>


namespace
{

const int s_btnClms = 3;

}


DataBrowserTableModel::DataBrowserTableModel(QObject * parent)
    : QAbstractTableModel(parent)
    , m_numDataObjects(0)
    , m_numAttributeVectors(0)
{
    m_icons.insert("rendered", QIcon(":/icons/painting.svg"));
    //m_icons.insert("notRendered", QIcon(":/icons/painting_faded.svg"));
    m_icons.insert("table", QIcon(":/icons/table.svg"));
    m_icons.insert("delete_red", QIcon(":/icons/delete_red.svg"));

    {
        QPixmap pixmap(":/icons/painting.svg"); QImage image = pixmap.toImage();
        for (int i = 0; i < pixmap.width(); ++i)
        {
            for (int j = 0; j < pixmap.height(); ++j)
            {
                QRgb pixel = image.pixel(i, j);
                int gray = qGray(pixel);
                image.setPixel(i, j, qRgba(gray, gray, gray, qAlpha(pixel)));
            };
        }
        m_icons.insert("notRendered", QIcon(QPixmap().fromImage(image)));
    }
}

int DataBrowserTableModel::rowCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return m_numDataObjects + m_numAttributeVectors;
}

int DataBrowserTableModel::columnCount(const QModelIndex &/*parent = QModelIndex()*/) const
{
    return s_btnClms + 5;
}

QVariant DataBrowserTableModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    int row = index.row();
    int column = index.column();

    if (row < m_numDataObjects)
        return data_dataObject(row, column, role);

    return data_attributeVector(row - m_numDataObjects, column, role);
}

QVariant DataBrowserTableModel::headerData(int section, Qt::Orientation orientation,
    int role /*= Qt::DisplayRole*/) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation != Qt::Orientation::Horizontal)
        return QVariant();

    switch (section)
    {
    case s_btnClms + 0: return "name";
    case s_btnClms + 1: return "data set type";
    case s_btnClms + 2: return "dimensions";
    case s_btnClms + 3: return "spatial bounds";
    case s_btnClms + 4: return "value range";
    }        

    return QVariant();
}

DataObject * DataBrowserTableModel::dataObjectAt(int row) const
{
    if (row < m_numDataObjects)
        return DataSetHandler::instance().dataSets()[row];

    row -= m_numDataObjects;
    assert(row < m_numAttributeVectors);

    return DataSetHandler::instance().rawVectors()[row];
}

DataObject * DataBrowserTableModel::dataObjectAt(const QModelIndex & index) const
{
    return dataObjectAt(index.row());
}

int DataBrowserTableModel::rowForDataObject(DataObject * dataObject) const
{
    if (RawVectorData * raw = dynamic_cast<RawVectorData *>(dataObject))
        return m_numDataObjects + DataSetHandler::instance().rawVectors().indexOf(raw);

    return DataSetHandler::instance().dataSets().indexOf(dataObject);
}

QList<DataObject *> DataBrowserTableModel::dataObjects(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
        if (DataObject * d = dataObjectAt(index.row()))
            result << d;

    return result;
}

QList<DataObject *> DataBrowserTableModel::dataSets(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
    {
        if (index.row() >= m_numDataObjects)
            continue;

        if (DataObject * d = dataObjectAt(index.row()))
            result << d;
    }

    return result;
}

QList<DataObject *> DataBrowserTableModel::rawVectors(QModelIndexList indexes)
{
    QList<DataObject *> result;

    for (const QModelIndex & index : indexes)
    {
        if (index.row() <= m_numDataObjects)
            continue;

        if (DataObject * d = dataObjectAt(index.row()))
            result << d;
    }

    return result;
}

int DataBrowserTableModel::numButtonColumns()
{
    return s_btnClms;
}

QString DataBrowserTableModel::componentName(int component, int numComponents)
{
    return numComponents <= 3
        ? QChar::fromLatin1('x' + component)
        : "[" + QString::number(component) + "]";
}

void DataBrowserTableModel::updateDataList(const QList<DataObject *> & visibleObjects)
{
    beginResetModel();

    const QList<DataObject *> & dataSets = DataSetHandler::instance().dataSets();
    const QList<RawVectorData *> & rawVectors = DataSetHandler::instance().rawVectors();
    m_numDataObjects = dataSets.size();
    m_numAttributeVectors = rawVectors.size();

    m_visibilities.clear();
    for (DataObject * dataObject : dataSets)
        m_visibilities.insert(dataObject, visibleObjects.contains(dataObject));
    for (RawVectorData * attr : rawVectors)
        m_visibilities.insert(attr, visibleObjects.contains(attr));

    endResetModel();
}

QVariant DataBrowserTableModel::data_dataObject(int row, int column, int role) const
{
    if (role == Qt::DecorationRole)
    {
        switch (column)
        {
        case 0:
            return m_icons["table"];
        case 1:
            return m_visibilities[dataObjectAt(row)]
                ? m_icons["rendered"]
                : m_icons["notRendered"];
        case 2:
            return m_icons["delete_red"];
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    const QList<DataObject *> & dataSets = DataSetHandler::instance().dataSets();
    assert(row < dataSets.size());

    DataObject * dataObject = dataSets.at(row);

    QString dataTypeName = dataObject->dataTypeName();

    switch (column)
    {
    case s_btnClms:
        return dataObject->name();
    case s_btnClms + 1:
        return dataObject->dataTypeName();
    case s_btnClms + 2:
        if (dataTypeName == "polygonal mesh")
            return
            QString::number(dataObject->dataSet()->GetNumberOfCells()) + " triangles, " +
            QString::number(dataObject->dataSet()->GetNumberOfPoints()) + " vertices";
        if (dataTypeName == "regular 2D grid")
        {
            ImageDataObject * imageData = static_cast<ImageDataObject*>(dataObject);
            return QString::number(imageData->dimensions()[0]) + "x" + QString::number(imageData->dimensions()[1]) + " values";
        }
        if (dataTypeName == "image profile")
        {
            return QString::number(static_cast<ImageProfileData *>(dataObject)->numberOfScalars()) + " values";
        }
        if (dataTypeName == "3D vector grid")
        {
            const int * dimensions = static_cast<VectorGrid3DDataObject*>(dataObject)->dimensions();
            return QString::number(dimensions[0]) + "x" + QString::number(dimensions[1]) + "x" + QString::number(dimensions[2]) + " vectors";
        }
        break;
    case s_btnClms + 3:
        return
            "x: " + QString::number(dataObject->bounds()[0]) + "; " + QString::number(dataObject->bounds()[1]) +
            ", y: " + QString::number(dataObject->bounds()[2]) + "; " + QString::number(dataObject->bounds()[3]) +
            ", z: " + QString::number(dataObject->bounds()[4]) + "; " + QString::number(dataObject->bounds()[5]);
    case s_btnClms + 4:
        if (dataTypeName == "polygonal mesh")
            return "";
        if (dataTypeName == "regular 2D grid")
        {
            const double * minMax = static_cast<ImageDataObject*>(dataObject)->minMaxValue();
            return QString::number(minMax[0]) + "; " + QString::number(minMax[1]);
        }
        if (dataTypeName == "image profile")
        {
            const double * range = static_cast<ImageProfileData *>(dataObject)->scalarRange();
            return QString::number(range[0]) + "; " + QString::number(range[1]);
        }
        if (dataTypeName == "3D vector grid")
        {
            VectorGrid3DDataObject * vectorGrid = static_cast<VectorGrid3DDataObject*>(dataObject);
            QString line;
            int numComponents = vectorGrid->numberOfComponents();
            for (int c = 0; c < numComponents; ++c)
            {
                line += componentName(c, numComponents) + ": " + QString::number(vectorGrid->scalarRange(c)[0]) + "; " + QString::number(vectorGrid->scalarRange(c)[1]) + ", ";
            }
            line.chop(2);
            return line;
        }
        break;
    }

    return QVariant();
}

QVariant DataBrowserTableModel::data_attributeVector(int row, int column, int role) const
{
    if (role == Qt::DecorationRole)
    {
        switch (column)
        {
        case 0:
            return m_icons["table"];
        case 2:
            return m_icons["delete_red"];
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    const QList<RawVectorData *> & rawVectors = DataSetHandler::instance().rawVectors();
    assert(row < rawVectors.size());

    RawVectorData * attributeVector = rawVectors.at(row);

    vtkDataArray * dataArray = attributeVector->dataArray();
    int numComponents = dataArray->GetNumberOfComponents();

    switch (column)
    {
    case s_btnClms:
        return attributeVector->name();
    case s_btnClms + 1:
        return QString::number(numComponents) + " component vector";
    case s_btnClms + 2:
        return QString::number(dataArray->GetNumberOfTuples()) + " tuples";
    case s_btnClms + 3:
        return "";
    case s_btnClms + 4:
    {
        QString ranges;
        double range[2];
        for (int c = 0; c < numComponents; ++c)
        {
            dataArray->GetRange(range, c);
            ranges += componentName(c, numComponents) + ": " + QString::number(range[0]) + "; " + QString::number(range[1]) + ", ";
        }
        ranges.remove(ranges.length() - 2, 2);
        return ranges;
    }
    }

    return QVariant();
}
