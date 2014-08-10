#pragma once

#include <QList>
#include <QMap>
#include <QString>

#include <vtkSmartPointer.h>

#include <core/core_api.h>


class vtkLookupTable;

class RenderedData;
class ScalarsForColorMapping;


/**
Sets up scalar to surface color mapping for rendered data and stores the configuration state.
Uses ScalarsForColorMapping to determine scalars that can be mapped on the supplied renderedData.
*/
class CORE_API ScalarToColorMapping
{
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
    const ScalarsForColorMapping * currentScalars() const;

    /** @return currently used gradient, in case it was previously set */
    vtkLookupTable * gradient();
    void setGradient(vtkLookupTable * gradient);

private:
    ScalarsForColorMapping * m_currentScalars();
    QList<RenderedData *> m_renderedData;

    QMap<QString, ScalarsForColorMapping *> m_scalars;

    QString m_currentScalarsName;
    vtkSmartPointer<vtkLookupTable> m_gradient;
};
