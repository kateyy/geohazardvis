#pragma once

#include <QAbstractTableModel>
#include <QList>
#include <QMap>
#include <QIcon>


class DataObject;


class DataBrowserTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    DataBrowserTableModel(QObject * parent = nullptr);

    void updateDataList(const QList<DataObject *> & visibleObjects);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    DataObject * dataObjectAt(int row) const;
    DataObject * dataObjectAt(const QModelIndex & index) const;

    QList<DataObject *> dataObjects(QModelIndexList indexes);
    QList<DataObject *> dataSets(QModelIndexList indexes);
    QList<DataObject *> rawVectors(QModelIndexList indexes);

    static int numButtonColumns();

private:
    QVariant data_dataObject(int row, int column, int role) const;
    QVariant data_attributeVector(int row, int column, int role) const;

private:
    QMap<const DataObject *, bool> m_visibilities;
    QMap<QString, QIcon> m_icons;
    int m_numDataObjects;
    int m_numAttributeVectors;
};
