#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <QObject>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;
class vtkScalarsToColors;

class AbstractVisualizedData;
class ColorBarRepresentation;
class ColorMappingData;
class GlyphColorMappingGlyphListener;


/**
 * Sets up scalar to surface color mapping for rendered data and stores the configuration state.
 *
 * Uses ColorMappingData to determine scalars that can be mapped on the supplied renderedData.
*/
class CORE_API ColorMapping : public QObject
{
    Q_OBJECT

public:
    ColorMapping();
    ~ColorMapping() override;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    /**
     * @return true only if scalar mappings are available for the current data sets.
     * Otherwise, setEnabled() will have no effect and the null mapping will be used.
     */
    bool scalarsAvailable() const;

    const std::vector<AbstractVisualizedData *> & visualizedData() const;

    /** List of scalar names that can be used with my rendered data */
    QStringList scalarsNames() const;
    std::vector<ColorMappingData *> scalars();
    std::vector<const ColorMappingData *> scalars() const;

    const QString & currentScalarsName() const;
    void setCurrentScalarsByName(const QString & scalarsName);
    void setCurrentScalarsByName(const QString & scalarsName, bool enableColorMapping);
    const ColorMappingData & currentScalars() const;
    ColorMappingData & currentScalars();

    /**
     * Access scalars by their name, but don't change current scalar selection.
     * @return nullptr if the name is not valid.
     */
    const ColorMappingData * scalarsByName(const QString & scalarsName) const;
    ColorMappingData * scalarsByName(const QString & scalarsName);

    /** @return gradient lookup table */
    vtkLookupTable * gradient();
    /** Convenience method, returns the same as gradient() */
    vtkScalarsToColors * scalarsToColors();
    const QString & gradientName() const;
    void setGradient(const QString & gradientName);

    /**
     * Manually specify a lookup table that will be used instead of a one fetched from the
     * GradientResourceManager. When a manually set gradient is used, users should not be able to
     * adjust the gradient in the UI.
     */
    void setManualGradient(vtkLookupTable & gradient);
    /** Use gradients identified by name again, see setGradient(). */
    void setUseDefaultGradients();
    bool usesManualGradient() const;

    bool currentScalarsUseMappingLegend() const;
    ColorBarRepresentation & colorBarRepresentation();

    /**
     * (Un-)register a visualization with this color mapping.
     * Must ONLY be called by the visualization itself.
     */
    void registerVisualizedData(AbstractVisualizedData * visualizedData);
    void unregisterVisualizedData(AbstractVisualizedData * visualizedData);

signals:
    void scalarsChanged();
    void currentScalarsChanged();
    void visualizedDataChanged();

private:
    /**
     * Setup a list of color mappings which are applicable to the list of rendered data.
     * Reuse most recently used scalars if possible
     */
    void setVisualizedData(const std::vector<AbstractVisualizedData *> & visualizedData);

    /** Reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableScalars();

    void updateCurrentMappingState(const QString & scalarsName, bool enabled);

    ColorMappingData & nullColorMapping() const;
    void releaseMappingData();

    void applyGradient(vtkLookupTable & gradient);

private:
    bool m_isEnabled;

    std::unique_ptr<GlyphColorMappingGlyphListener> m_glyphListener;
    std::unique_ptr<ColorBarRepresentation> m_colorBarRepresenation;

    std::vector<AbstractVisualizedData *> m_visualizedData;

    std::map<QString, std::unique_ptr<ColorMappingData>> m_data;
    mutable std::unique_ptr<ColorMappingData> m_nullColorMapping;
    mutable std::mutex m_nullMappingMutex;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;
    bool m_useManualGradient;
    QString m_gradientName;

private:
    Q_DISABLE_COPY(ColorMapping)
};
