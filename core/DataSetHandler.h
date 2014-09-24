#pragma once

#include <QObject>

#include <core/core_api.h>

template <typename T> class QList;

class DataObject;
class AttributeVectorData;


class CORE_API DataSetHandler : public QObject
{
    Q_OBJECT

public:
    static DataSetHandler & instance();

    void addData(QList<DataObject *> dataObjects);
    void deleteData(QList<DataObject *> dataObjects);

    QList<DataObject *> dataObjects();
    QList<AttributeVectorData *> attributeVectors();

signals:
    void dataObjectsChanged();
    void attributeVectorsChanged();

private:
    DataSetHandler();
    ~DataSetHandler();
};
