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

    void addDataObject(DataObject * dataObject);
    void removeDataObject(DataObject * dataObject);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    DataObject * dataObjectAt(int row);

private:
    QList<DataObject *> m_dataObjects;
    QMap<const DataObject *, bool> m_visibilities;

    QMap<QString, QIcon> m_icons;

};