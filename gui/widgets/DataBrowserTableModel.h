#pragma once

#include <QAbstractTableModel>
#include <QMap>


class QIcon;

class DataObject;
class DataSetHandler;


class DataBrowserTableModel : public QAbstractTableModel
{
public:
    explicit DataBrowserTableModel(QObject * parent = nullptr);
    ~DataBrowserTableModel() override;

    void setDataSetHandler(const DataSetHandler * dataSetHandler);

    void updateDataList(const QList<DataObject *> & visibleObjects);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    DataObject * dataObjectAt(int row) const;
    DataObject * dataObjectAt(const QModelIndex & index) const;
    int rowForDataObject(DataObject * dataObject) const;

    QList<DataObject *> dataObjects(QModelIndexList indexes);
    QList<DataObject *> dataSets(QModelIndexList indexes);
    QList<DataObject *> rawVectors(QModelIndexList indexes);

    static int numButtonColumns();

    /** nice name for each component (x, y, z for up to three components, [0].. for more) */
    static QString componentName(int component, int numComponents);

private:
    QVariant data_dataObject(int row, int column, int role) const;
    QVariant data_attributeVector(int row, int column, int role) const;

private:
    const DataSetHandler * m_dataSetHandler;
    QMap<const DataObject *, bool> m_visibilities;
    QMap<QString, QIcon> m_icons;
    QList<QMetaObject::Connection> m_dataObjectConnections;
    int m_numDataObjects;
    int m_numAttributeVectors;

private:
    Q_DISABLE_COPY(DataBrowserTableModel)
};
