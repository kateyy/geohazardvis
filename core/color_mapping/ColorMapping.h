#pragma once

#include <map>
#include <memory>

#include <QList>
#include <QMap>
#include <QString>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;
class vtkScalarsToColors;

class AbstractVisualizedData;
class ColorBarRepresentation;
class ColorMappingData;
class GlyphColorMappingGlyphListener;


/**
Sets up scalar to surface color mapping for rendered data and stores the configuration state.
Uses ColorMappingData to determine scalars that can be mapped on the supplied renderedData.
*/
class CORE_API ColorMapping : public QObject
{
    Q_OBJECT

public:
    ColorMapping();
    ~ColorMapping() override;

    const QList<AbstractVisualizedData *> & visualizedData() const;

    /** list of scalar names that can be used with my rendered data */
    QStringList scalarsNames() const;

    const QString & currentScalarsName() const;
    void setCurrentScalarsByName(const QString & scalarsName);
    ColorMappingData * currentScalars();
    const ColorMappingData * currentScalars() const;

    /** @return gradient lookup table */
    vtkLookupTable * gradient();
    /** Convenience method, returns the same as gradient() */
    vtkScalarsToColors * scalarsToColors();
    const QString & gradientName() const;
    void setGradient(const QString & gradientName);

    bool currentScalarsUseMappingLegend() const;
    ColorBarRepresentation & colorBarRepresentation();

    /** (un-)register a visualization with this color mapping.
      * Must ONLY be called by the visualization itself. */
    void registerVisualizedData(AbstractVisualizedData * visualizedData);
    void unregisterVisualizedData(AbstractVisualizedData * visualizedData);

signals:
    void scalarsChanged();
    void currentScalarsChanged();
    void visualizedDataChanged();

private:
    /** setup a list of color mappings which are applicable to the list of rendered data
      * reuse lastly used scalars if possible */
    void setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData);

    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableScalars();

private:
    std::unique_ptr<GlyphColorMappingGlyphListener> m_glyphListener;
    std::unique_ptr<ColorBarRepresentation> m_colorBarRepresenation;

    QList<AbstractVisualizedData *> m_visualizedData;

    std::map<QString, std::unique_ptr<ColorMappingData>> m_data;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;
    QString m_gradientName;
};
