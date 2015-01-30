#pragma once

#include <QMap>

#include <core/core_api.h>

class QString;
class DataObject;
class ImageDataObject;
class VectorGrid3DDataObject;


class CORE_API Exporter
{
public:
    static bool exportData(DataObject * data, const QString & fileName);

    static bool isExportSupported(DataObject * data);
    static QString formatFilter(DataObject * data);
    static const QMap<QString, QString> & formatFilters();

    static bool exportImage(ImageDataObject * image, const QString & fileName);
    static bool exportVectorGrid3D(VectorGrid3DDataObject * grid, const QString & fileName);
};
