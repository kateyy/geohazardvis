#include "DataSetHandler.h"

#include <cassert>

#include <QList>
#include <QMutex>

#include <core/data_objects/RawVectorData.h>


using std::unique_ptr;
using std::vector;

namespace
{
vector<unique_ptr<DataObject>> * s_dataSets = nullptr;
vector<unique_ptr<DataObject>> * s_rawVectors = nullptr;

// convenience etc
QList<DataObject *> * s_dataSetsQt = nullptr;
QList<RawVectorData *> * s_rawVectorsQt = nullptr;
}


DataSetHandler::DataSetHandler()
    : QObject()
    , m_mutex(std::make_unique<QMutex>())
{
    s_dataSets = new vector<unique_ptr<DataObject>>();
    s_rawVectors = new vector<unique_ptr<DataObject>>();

    s_dataSetsQt = new QList<DataObject *>();
    s_rawVectorsQt = new QList<RawVectorData *>();
}

DataSetHandler::~DataSetHandler()
{
    m_mutex->lock();

    delete s_dataSets;
    delete s_rawVectors;

    delete s_dataSetsQt;
    delete s_rawVectorsQt;

    m_mutex->unlock();
}

DataSetHandler & DataSetHandler::instance()
{
    static DataSetHandler instance;

    return instance;
}

void DataSetHandler::takeData(std::unique_ptr<DataObject> dataObject)
{
    // passing by value requires callers to use "unique_ptr<..> obj; takeData(move(obj));"
    // thus, it's more explicit "move" on the caller's side

    // initializer list don't support move semantics, so we need a push_back(move...)) here
    std::vector<std::unique_ptr<DataObject>> vec;
    vec.push_back(std::move(dataObject));
    takeData(move(vec));
}

void DataSetHandler::takeData(std::vector<std::unique_ptr<DataObject>> dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    {
        QMutexLocker lock(m_mutex.get());
        for (auto && dataObject : dataObjects)
        {
            if (dataObject->dataSet())
            {
                *s_dataSetsQt << dataObject.get();
                s_dataSets->push_back(std::move(dataObject));
                dataChanged = true;
            }
            else
            {
                auto rawData = dynamic_cast<RawVectorData *>(dataObject.get());
                assert(rawData);
                if (!rawData)   // can't handle that, just delete it   
                    continue;

                *s_rawVectorsQt << rawData;
                s_rawVectors->push_back(std::move(dataObject));
                rawDataChanged = true;
            }
        }
    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (rawDataChanged)
        emit rawVectorsChanged();
}

void DataSetHandler::deleteData(const QList<DataObject *> & dataObjects)
{
    bool dataChanged = false, rawDataChanged = false;

    {
        QMutexLocker locker(m_mutex.get());
        for (DataObject * dataObject : dataObjects)
        {
            if (dataObject->dataSet())
            {
                auto it = s_dataSets->begin();
                for (; it != s_dataSets->end(); ++it)
                    if (it->get() == dataObject)
                        break;

                if (it == s_dataSets->end())
                    continue;

                s_dataSets->erase(it);
                s_dataSetsQt->removeOne(dataObject);
                dataChanged = true;
            }
            else
            {
                auto it = s_rawVectors->begin();
                for (; it != s_rawVectors->end(); ++it)
                    if (it->get() == dataObject)
                        break;

                if (it == s_rawVectors->end())
                    continue;

                s_rawVectors->erase(it);
                s_rawVectorsQt->removeOne(static_cast<RawVectorData *>(dataObject));
                rawDataChanged = true;
            }

        }
    }

    if (dataChanged)
        emit dataObjectsChanged();
    if (rawDataChanged)
        emit rawVectorsChanged();
}

const QList<DataObject *> & DataSetHandler::dataSets()
{
    QMutexLocker lock(m_mutex.get());

    return *s_dataSetsQt;
}

const QList<RawVectorData *> & DataSetHandler::rawVectors()
{
    QMutexLocker lock(m_mutex.get());

    return *s_rawVectorsQt;
}
