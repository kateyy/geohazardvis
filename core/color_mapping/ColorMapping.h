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
class vtkScalarBarActor;

class AbstractVisualizedData;
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
    ColorMapping(QObject * parent = nullptr);
    ~ColorMapping() override;

    /** setup a list of color mappings which are applicable to the list of rendered data
      * reuse lastly used scalars if possible */
    void setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData);

    void clear();

    /** list of scalar names that can be used with my rendered data */
    QList<QString> scalarsNames() const;
    /** list of scalars that can be used with my rendered data */
    const QMap<QString, const ColorMappingData *> & scalars() const;

    QString currentScalarsName() const;
    void setCurrentScalarsByName(const QString & scalarsName);
    ColorMappingData * currentScalars();
    const ColorMappingData * currentScalars() const;

    void scalarsSetDataComponent(int component);

    /** @return gradient lookup table
      * This is empty or a copy of the table passed by setGradient. */
    vtkLookupTable * gradient();
    /** @return pointer to the gradient that was passed via setGradient
      * Used to compare the local gradient copy with the original object. */
    vtkLookupTable * originalGradient();
    void setGradient(vtkLookupTable * gradient);

    vtkScalarBarActor * colorMappingLegend();
    bool currentScalarsUseMappingLegend() const;
    /** get/set visibility of the color mapping legend.
        It is always hidden if the current scalars don't use the color mapping. */
    bool colorMappingLegendVisible() const;
    void setColorMappingLegendVisible(bool visible);

signals:
    void scalarsChanged();
    void colorLegendVisibilityChanged(bool visible);

private:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableScalars();
    void updateLegendVisibility();

private:
    std::unique_ptr<GlyphColorMappingGlyphListener> m_glyphListener;

    QList<AbstractVisualizedData *> m_visualizedData;

    std::map<QString, std::unique_ptr<ColorMappingData>> m_scalars;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;
    vtkLookupTable * m_originalGradient;

    vtkSmartPointer<vtkScalarBarActor> m_colorMappingLegend;
    bool m_colorMappingLegendVisible;
};
