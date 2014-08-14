#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QObject>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;
class vtkScalarBarActor;

class RenderedData;
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

    /** setup a list of scalar mappings which are applicable to the list of rendered data
      * reuse lastly used scalars if possible */
    void setRenderedData(const QList<RenderedData *> & renderedData);

    void clear();

    /** list of scalar names that can be used with my rendered data */
    QList<QString> scalarsNames() const;
    /** list of scalars that can be used with my rendered data */
    const QMap<QString, const ScalarsForColorMapping *> & scalars() const;

    QString currentScalarsName() const;
    void setCurrentScalarsByName(QString scalarsName);
    ScalarsForColorMapping * currentScalars();

    /** @return currently used gradient, in case it was previously set */
    vtkLookupTable * gradient();
    void setGradient(vtkLookupTable * gradient);

    vtkScalarBarActor * colorMappingLegend();

private slots:
    /** apply changed min/max from scalar to color mapping to gradient lookup table */
    void updateGradientValueRange();

private:
    QList<RenderedData *> m_renderedData;

    QMap<QString, ScalarsForColorMapping *> m_scalars;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;

    vtkSmartPointer<vtkScalarBarActor> m_colorMappingLegend;
};
