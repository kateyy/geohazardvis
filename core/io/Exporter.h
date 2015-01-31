#pragma once

#include <QMap>

#include <core/core_api.h>

class QString;
class DataObject;
class ImageDataObject;


class CORE_API Exporter
{
public:
    static bool exportData(DataObject * data, const QString & fileName);

    static bool isExportSupported(DataObject * data);
    static QString formatFilter(DataObject * data);
    static const QMap<QString, QString> & formatFilters();

    static bool exportImageFormat(ImageDataObject * image, const QString & fileName);
    static bool exportVTKXMLPolyData(DataObject * polyData, const QString & fileName);
    static bool exportVTKXMLImageData(DataObject * image, const QString & fileName);
};
