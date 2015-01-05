#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;
class vtkScalarBarActor;

class AbstractVisualizedData;
class ScalarsForColorMapping;


/**
Sets up scalar to surface color mapping for rendered data and stores the configuration state.
Uses ScalarsForColorMapping to determine scalars that can be mapped on the supplied renderedData.
*/
class CORE_API ScalarToColorMapping : public QObject
{
    Q_OBJECT

public:
    ScalarToColorMapping();
    ~ScalarToColorMapping() override;

    /** setup a list of scalar mappings which are applicable to the list of rendered data
      * reuse lastly used scalars if possible */
    void setVisualizedData(const QList<AbstractVisualizedData *> & visualizedData);

    void clear();

    /** list of scalar names that can be used with my rendered data */
    QList<QString> scalarsNames() const;
    /** list of scalars that can be used with my rendered data */
    const QMap<QString, const ScalarsForColorMapping *> & scalars() const;

    QString currentScalarsName() const;
    void setCurrentScalarsByName(QString scalarsName);
    ScalarsForColorMapping * currentScalars();
    const ScalarsForColorMapping * currentScalars() const;

    void scalarsSetDataComponent(vtkIdType component);

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

private slots:
    /** reread the data set list provided by the DataSetHandler for new/deleted data */
    void updateAvailableScalars();
    void updateLegendVisibility();

private:
    QList<AbstractVisualizedData *> m_visualizedData;

    QMap<QString, ScalarsForColorMapping *> m_scalars;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;
    vtkLookupTable * m_originalGradient;

    vtkSmartPointer<vtkScalarBarActor> m_colorMappingLegend;
    bool m_colorMappingLegendVisible;
};
