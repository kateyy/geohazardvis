#pragma once

#include <QObject>

#include <core/core_api.h>

template <typename T> class QList;

class DataObject;


class CORE_API DataSetHandler : public QObject
{
    Q_OBJECT

public:
    static DataSetHandler & instance();

    void add(DataObject * dataObject);
    void deleteObject(DataObject * dataObject);

    QList<DataObject *> dataObjects();

private:
    DataSetHandler();
    ~DataSetHandler();
};
