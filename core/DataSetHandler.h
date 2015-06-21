#pragma once

#include <memory>
#include <vector>

#include <QObject>

#include <core/core_api.h>

template <typename T> class QList;
class QMutex;

class DataObject;
class RawVectorData;


class CORE_API DataSetHandler : public QObject
{
    Q_OBJECT

public:
    static DataSetHandler & instance();

    void takeData(std::unique_ptr<DataObject> dataObject);
    void takeData(std::vector<std::unique_ptr<DataObject>> dataObjects);
    void deleteData(const QList<DataObject *> & dataObjects);

    const QList<DataObject *> & dataSets();
    const QList<RawVectorData *> & rawVectors();

signals:
    void dataObjectsChanged();
    void rawVectorsChanged();

private:
    DataSetHandler();
    ~DataSetHandler();
    DataSetHandler(const DataSetHandler&) = delete;
    void operator=(const DataSetHandler&) = delete;

    std::unique_ptr<QMutex> m_mutex;
};
