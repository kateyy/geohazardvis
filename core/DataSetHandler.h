#pragma once

#include <QObject>

#include <core/core_api.h>

template <typename T> class QList;

class DataObject;
class RawVectorData;


class CORE_API DataSetHandler : public QObject
{
    Q_OBJECT

public:
    static DataSetHandler & instance();

    void addData(QList<DataObject *> dataObjects);
    void deleteData(QList<DataObject *> dataObjects);

    const QList<DataObject *> & dataSets();
    const QList<RawVectorData *> & rawVectors();

signals:
    void dataObjectsChanged();
    void rawVectorsChanged();

private:
    DataSetHandler();
    ~DataSetHandler();
};
