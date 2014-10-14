#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QObject>

#include <core/core_api.h>


class RenderedData;
class VectorMappingData;


/**
Sets up vector to surface mapping for a data object and stores the configuration state.
Uses VectorMappingData to determine vectors that can be mapped on the supplied data.
*/
class CORE_API VectorMapping : public QObject
{
    Q_OBJECT

public:
    VectorMapping(RenderedData * renderedData);
    ~VectorMapping() override;

    /** names of vectors that can be used with my data */
    QList<QString> vectorNames() const;
    /** list of vectors that can be used with my data */
    const QMap<QString, VectorMappingData *> & vectors() const;

    const RenderedData * renderedData() const;

signals:
    void vectorsChanged();

private slots:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableVectors();

private:
    RenderedData * m_renderedData;

    QMap<QString, VectorMappingData *> m_vectors;
};
