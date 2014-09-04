#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QObject>

#include <core/core_api.h>


class RenderedData;
class VectorsForSurfaceMapping;


/**
Sets up vector to surface mapping for a data object and stores the configuration state.
Uses VectorsForSurfaceMapping to determine vectors that can be mapped on the supplied data.
*/
class CORE_API VectorsToSurfaceMapping : public QObject
{
    Q_OBJECT

public:
    VectorsToSurfaceMapping(RenderedData * renderedData);

    /** names of vectors that can be used with my data */
    QList<QString> vectorNames() const;
    /** list of vectors that can be used with my data */
    const QMap<QString, VectorsForSurfaceMapping *> & vectors() const;

private:
    RenderedData * m_renderedData;

    QMap<QString, VectorsForSurfaceMapping *> m_vectors;
};
