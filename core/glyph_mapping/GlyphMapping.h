#pragma once

#include <map>
#include <memory>

#include <QObject>

#include <core/core_api.h>


template<typename T> class QList;
class QString;
class QStringList;

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
    explicit GlyphMapping(RenderedData & renderedData);
    ~GlyphMapping() override;

    /** names of vectors that can be used with my data */
    QStringList vectorNames() const;
    /** list of vector data that can be used with my data */
    QList<GlyphMappingData *> vectors() const;

    const RenderedData & renderedData() const;

signals:
    void vectorsChanged();

private:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableVectors();

private:
    RenderedData & m_renderedData;

    std::map<QString, std::unique_ptr<GlyphMappingData>> m_vectors;
};
