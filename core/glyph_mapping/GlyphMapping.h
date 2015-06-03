#pragma once

#include <QList>
#include <QMap>
#include <QObject>

#include <core/core_api.h>


class QString;

class RenderedData;
class GlyphMappingData;


/**
Sets up vector to surface mapping for a data object and stores the configuration state.
Uses GlyphMappingData to determine vectors that can be mapped on the supplied data.
*/
class CORE_API GlyphMapping : public QObject
{
    Q_OBJECT

public:
    GlyphMapping(RenderedData * renderedData);
    ~GlyphMapping() override;

    /** names of vectors that can be used with my data */
    QList<QString> vectorNames() const;
    /** list of vector data that can be used with my data */
    const QMap<QString, GlyphMappingData *> & vectors() const;

    const RenderedData * renderedData() const;

signals:
    void vectorsChanged();

private slots:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableVectors();

private:
    RenderedData * m_renderedData;

    QMap<QString, GlyphMappingData *> m_vectors;
};
